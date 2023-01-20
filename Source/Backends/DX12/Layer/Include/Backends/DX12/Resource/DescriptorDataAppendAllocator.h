#pragma once

// Layer
#include <Backends/DX12/Allocation/DeviceAllocator.h>
#include "DescriptorDataSegment.h"

// Common
#include <Common/ComRef.h>

// Std
#include <vector>

class DescriptorDataAppendAllocator {
public:
    DescriptorDataAppendAllocator(const ComRef<DeviceAllocator>& allocator) : allocator(allocator) {

    }

    /// Set the chunk
    /// \param segmentEntry segment to be bound to
    void SetChunk(const DescriptorDataSegmentEntry& segmentEntry) {
        // Set inherited width
        chunkSize = segmentEntry.allocation.resource->GetDesc().Width / sizeof(uint32_t);

        // Add entry
        DescriptorDataSegmentEntry& entry = segment.entries.emplace_back();
        entry.allocation = segmentEntry.allocation;

        // Opaque handle
        void* mappedOpaque{nullptr};

        // Map range
        D3D12_RANGE range;
        range.Begin = 0;
        range.End = sizeof(uint32_t) * chunkSize;
        entry.allocation.resource->Map(0, &range, &mappedOpaque);

        // Set mapped
        mapped = static_cast<uint32_t*>(mappedOpaque);
    }

    /// Begin a new segment
    /// \param rootCount number of root parameters
    void BeginSegment(uint32_t rootCount) {
        pendingRootCount = rootCount;
        pendingRoll = true;
    }

    /// Set a root value
    /// \param offset current root offset
    /// \param value value at root offset
    void Set(uint32_t offset, uint32_t value) {
        if (pendingRoll) {
            RollChunk();
        }

        ASSERT(offset < mappedSegmentLength, "Out of bounds descriptor segment offset");
        mapped[mappedOffset + offset] = value;
    }

    /// Has this allocator been rolled? i.e. a new segment has begun
    /// \return
    bool HasRolled() const {
        return !pendingRoll;
    }

    /// Commit all changes for the GPU
    void Commit() {
        if (!mapped) {
            return;
        }

        // Map range
        D3D12_RANGE range;
        range.Begin = 0;
        range.End = sizeof(uint32_t) * chunkSize;
        segment.entries.back().allocation.resource->Unmap(0, &range);
    }

    /// Get the current segment address
    /// \return
    D3D12_GPU_VIRTUAL_ADDRESS GetSegmentVirtualAddress() {
        return segment.entries.back().allocation.resource->GetGPUVirtualAddress() + mappedOffset;
    }

    /// Release the segment
    /// \return internal segment, ownership acquired
    DescriptorDataSegment ReleaseSegment() {
        // Reset internal state
        mappedOffset = 0;
        mappedSegmentLength = 0;
        chunkSize = 0;
        mapped = nullptr;

        // Release the segment
        return std::move(segment);
    }

private:
    /// Roll the current chunk
    void RollChunk() {
        // Advance current offset
        mappedOffset += std::max<size_t>(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, mappedSegmentLength);

        // Out of memory?
        if (mappedOffset + pendingRootCount >= chunkSize) {
            // Growth factor of 1.5
            uint64_t nextSize = std::max<size_t>(64'000, static_cast<size_t>(chunkSize * 1.5f));

            // Create new chunk
            CreateChunk(nextSize);
        }

        // Set next roll length
        mappedSegmentLength = pendingRootCount;
        pendingRoll = false;
    }

    /// Create a new chunk
    void CreateChunk(uint64_t size) {
        // Release existing chunk if needed
        if (mapped) {
            // Map range
            D3D12_RANGE range;
            range.Begin = 0;
            range.End = sizeof(uint32_t) * chunkSize;
            segment.entries.back().allocation.resource->Unmap(0, &range);
        }

        // Set new size
        chunkSize = size;

        // Reset
        mappedOffset = 0;
        mappedSegmentLength = 0;

        // Mapped description
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = 0;
        desc.Width = sizeof(uint32_t) * chunkSize;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.MipLevels = 1;
        desc.SampleDesc.Quality = 0;
        desc.SampleDesc.Count = 1;

        // Allocate buffer data on host, let the drivers handle page swapping
        DescriptorDataSegmentEntry segmentEntry;
        segmentEntry.allocation = allocator->Allocate(desc, AllocationResidency::Host);
        SetChunk(segmentEntry);
    }

private:
    /// Current mapping offset for the segment
    size_t mappedOffset{0};

    /// Current segment length
    size_t mappedSegmentLength{0};

    /// Total chunk size
    size_t chunkSize{0};

    /// Root count requested for the next roll
    uint32_t pendingRootCount{0};

    /// Any pending roll?
    bool pendingRoll{true};

    /// Device allocator
    ComRef<DeviceAllocator> allocator;

private:
    /// Current data segment, to be released later
    DescriptorDataSegment segment;

    /// Current mapped address of the segment
    uint32_t* mapped{nullptr};
};
