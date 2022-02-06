#pragma once

// Common
#include <Common/Containers/ReferenceObject.h>

// Backend
#include <Backend/ShaderExportTypeInfo.h>

// Layer
#include <Backends/Vulkan/Allocation/MirrorAllocation.h>

// Std
#include <vector>

/// A single allocation allocation
struct ShaderExportStreamInfo {
    /// Type info of the originating message stream
    ShaderExportTypeInfo typeInfo;

    /// Descriptor object
    VkBuffer buffer{VK_NULL_HANDLE};

    /// View
    VkBufferView view{VK_NULL_HANDLE};

    /// Data allocation
    MirrorAllocation allocation;
};

/// A batch of counters (for each stream), used for a single allocation
struct ShaderExportSegmentCounterInfo {
    /// Descriptor objects
    VkBuffer buffer{VK_NULL_HANDLE};
    VkBuffer bufferHost{VK_NULL_HANDLE};

    /// View
    VkBufferView view{VK_NULL_HANDLE};

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
