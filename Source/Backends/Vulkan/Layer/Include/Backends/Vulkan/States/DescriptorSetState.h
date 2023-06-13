#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/Resource/PhysicalResourceSegment.h>

// Common
#include "Common/Containers/ReferenceObject.h"

// Std
#include <cstdint>
#include <vector>

// Forward declarations
struct DeviceDispatchTable;

struct DescriptorSetState {
    /// Backwards reference
    DeviceDispatchTable* table;

    /// User pipeline
    VkDescriptorSet object{VK_NULL_HANDLE};

    /// Allocated segment
    PhysicalResourceSegmentID segmentID{0};

    /// Precomputed PRMT offsets
    std::vector<uint32_t> prmtOffsets;

    /// Linear swap index
    uint32_t poolSwapIndex{0};

    /// Unique identifier, unique for the type
    uint64_t uid;
};
