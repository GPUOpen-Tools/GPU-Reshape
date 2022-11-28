#pragma once

// Layer
#include "Backends/Vulkan/Vulkan.h"
#include "PipelineLayoutBindingInfo.h"

// Common
#include "Common/Containers/ReferenceObject.h"

// Std
#include <cstdint>
#include <vector>

// Forward declarations
struct DeviceDispatchTable;

struct PipelineLayoutState : public ReferenceObject {
    /// Reference counted destructor
    virtual ~PipelineLayoutState();

    /// Backwards reference
    DeviceDispatchTable* table;

    /// User pipeline
    VkPipelineLayout object{VK_NULL_HANDLE};

    /// Has this layout exhausted all its user slots?
    bool exhausted{false};

    /// Combined pipeline hash
    uint64_t compatabilityHash{0};

    /// Compatability hashes for all descriptor set layouts, ordered by bind order
    std::vector<uint64_t> compatabilityHashes;

    /// Dynamic offsets for all descriptor set layouts, ordered by bind order
    std::vector<uint32_t> descriptorDynamicOffsets;

    /// Number of descriptor sets for the user
    uint32_t boundUserDescriptorStates{0};

    /// Push constant offset
    uint32_t userPushConstantOffset{0};

    /// Unique identifier, unique for the type
    uint64_t uid;
};
