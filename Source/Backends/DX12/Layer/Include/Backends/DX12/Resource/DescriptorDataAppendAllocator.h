// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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
    DescriptorDataAppendAllocator(const Allocators& allocators, const ComRef<DeviceAllocator>& allocator) : allocator(allocator), segment(allocators) {

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
#ifndef NDEBUG
        localSegmentBindMask = 0u;
#endif // NDEBUG
        
        // Clear mapped data
        std::memset(mapped, 0x0, chunkSize * sizeof(uint32_t));
    }

    /// Begin a new segment
    /// \param rootCount number of root parameters
    void BeginSegment(uint32_t rootCount, bool migrateData) {
        migrateLastSegment = migrateData && mappedSegmentLength == rootCount;
        pendingRootCount = rootCount;
        pendingRoll = true;
    }

    /// Set a root value
    /// \param offset current root offset
    /// \param debugBindMask the binding index to be used, only for debugging
    /// \param value value at root offset
    void Set(uint32_t offset, uint32_t debugBindMask, uint32_t value) {
        if (pendingRoll) {
            RollChunk();
        }

#ifndef NDEBUG
        localSegmentBindMask |= (1ull << debugBindMask);
#endif // NDEBUG
        
        ASSERT(offset < mappedSegmentLength, "Out of bounds descriptor segment offset");
        mapped[mappedOffset + offset] = value;
    }

    /// Set a root value
    /// \param offset current root offset
    /// \param debugBindMask the binding index to be used, only for debugging
    /// \param value value at root offset, must be dword aligned, all dwords are inserted linearly
    template<typename T>
    void Set(uint32_t offset, uint32_t debugBindMask, const T& value) {
        if (pendingRoll) {
            RollChunk();
        }

        static_assert(sizeof(value) % sizeof(uint32_t) == 0, "Non-dword aligned value");
        static constexpr uint32_t kDWordCount = sizeof(value) / sizeof(uint32_t);
        
#ifndef NDEBUG
        localSegmentBindMask |= (1ull << debugBindMask);
#endif // NDEBUG
        
        ASSERT(offset + kDWordCount <= mappedSegmentLength, "Out of bounds descriptor segment offset");
        std::memcpy(mapped + mappedOffset + offset, &value, sizeof(value));
    }

    /// Manually roll the chunk
    void ConditionalRoll() {
        if (pendingRoll) {
            RollChunk();
        }
    }

#ifndef NDEBUG
    /// Validate current mask against another
    void ValidateAgainst(uint64_t mask) {
        ASSERT((localSegmentBindMask & mask) == mask, "Lost descriptor data");
    }

    /// Get the binding mask
    uint64_t GetBindMask() const {
        return localSegmentBindMask;
    }

    /// Get a value
    uint64_t Get(uint32_t offset) const {
        return mapped[mappedOffset + offset];
    }
#endif // NDEBUG

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
        if (segment.entries.empty()) {
            return 0ull;
        }
        
        return segment.entries.back().allocation.resource->GetGPUVirtualAddress() + mappedOffset * sizeof(uint32_t);
    }

    /// Release the segment
    /// \return internal segment, ownership acquired
    DescriptorDataSegment ReleaseSegment() {
        // Reset internal state
        mappedOffset = 0;
        mappedSegmentLength = 0;
        chunkSize = 0;
        mapped = nullptr;

#ifndef NDEBUG
        localSegmentBindMask = 0u;
#endif // NDEBUG

        // Release the segment
        return std::move(segment);
    }

private:
    /// Roll the current chunk
    void RollChunk() {
        // Advance current offset
        uint64_t nextMappedOffset = mappedOffset + mappedSegmentLength;

        // Align offset
        constexpr uint64_t alignSub1 = (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT / sizeof(uint32_t)) - 1;
        nextMappedOffset = (nextMappedOffset + alignSub1) & ~alignSub1;

#ifndef NDEBUG
        uint64_t lastSegmentBindMask = localSegmentBindMask;
        localSegmentBindMask = 0u;
#endif // NDEBUG

        // Out of memory?
        if (nextMappedOffset + pendingRootCount >= chunkSize) {
            // Copy last chunk if migration is requested
            uint32_t* lastChunkDwords = ALLOCA_ARRAY(uint32_t, mappedSegmentLength);
            if (migrateLastSegment) {
                std::memcpy(lastChunkDwords, mapped + mappedOffset, sizeof(uint32_t) * mappedSegmentLength);
            }

            // Growth factor of 1.5
            uint64_t nextSize = std::max<size_t>(64'000, static_cast<size_t>(chunkSize * 1.5f));

            // Create new chunk
            uint64_t lastSegmentLength = mappedSegmentLength;
            CreateChunk(nextSize);

            // Migrate last segment?
            if (migrateLastSegment) {
                ASSERT(pendingRootCount == lastSegmentLength, "Requested migration with mismatched root counts");
                std::memcpy(mapped, lastChunkDwords, sizeof(uint32_t) * lastSegmentLength);

#ifndef NDEBUG
                localSegmentBindMask = lastSegmentBindMask;
#endif // NDEBUG
            }
        } else {
            // Migrate last segment?
            if (mappedSegmentLength == pendingRootCount && migrateLastSegment) {
                std::memcpy(mapped + nextMappedOffset, mapped + mappedOffset, sizeof(uint32_t) * mappedSegmentLength);

#ifndef NDEBUG
                localSegmentBindMask = lastSegmentBindMask;
#endif // NDEBUG
            }
            
            // Set new offset
            mappedOffset = nextMappedOffset;
        }

        // Set next roll length
        mappedSegmentLength = pendingRootCount;
        pendingRoll = false;
        migrateLastSegment = false;
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

#ifndef NDEBUG
        segmentEntry.allocation.resource->SetName(L"AppendData");
#endif // NDEBUG
    }

#ifndef NDEBUG
    /// Current validation binding mask
    uint64_t localSegmentBindMask{0u};
#endif // NDEBUG

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

    /// Should the last segment be migrated on rolls?
    bool migrateLastSegment{false};

    /// Device allocator
    ComRef<DeviceAllocator> allocator;

private:
    /// Current data segment, to be released later
    DescriptorDataSegment segment;

    /// Current mapped address of the segment
    uint32_t* mapped{nullptr};
};
