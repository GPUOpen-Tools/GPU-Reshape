#pragma once

// Common
#include <Common/Containers/ReferenceObject.h>

// Backend
#include <Backend/ShaderExportTypeInfo.h>

// Layer
#include <Backends/DX12/Allocation/MirrorAllocation.h>

// Std
#include <vector>

/// A single allocation allocation
struct ShaderExportStreamInfo {
    /// Type info of the originating message stream
    ShaderExportTypeInfo typeInfo;

    /// Descriptor object
    ID3D12Resource* buffer{nullptr};

    /// Unordered view
    D3D12_UNORDERED_ACCESS_VIEW_DESC view{};

    /// Data allocation
    MirrorAllocation allocation;

    /// Actual byte size of the buffer (not allocation)
    uint64_t byteSize{0};
};

/// A batch of counters (for each stream), used for a single allocation
struct ShaderExportSegmentCounterInfo {
    /// Unordered view
    D3D12_UNORDERED_ACCESS_VIEW_DESC view{};

    /// Counter allocation
    MirrorAllocation allocation;
};

/// A single allocation, partitioning is up to the allocation modes
struct ShaderExportSegmentInfo {
    /// Stream container, will reach stable size after a set submissions
    std::vector<ShaderExportStreamInfo> streams;

    /// Counter batch
    ShaderExportSegmentCounterInfo counter;
};
