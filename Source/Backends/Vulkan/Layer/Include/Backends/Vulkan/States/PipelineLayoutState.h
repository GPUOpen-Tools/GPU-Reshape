#pragma once

// Layer
#include "Backends/Vulkan/Vulkan.h"
#include "Backends/Vulkan/States/PipelineLayoutPhysicalMapping.h"
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

    /// Physical mapping
    PipelineLayoutPhysicalMapping physicalMapping;

    /// Combined pipeline hash
    uint64_t compatabilityHash{0};

    /// Compatability hashes for all descriptor set layouts, ordered by bind order
    std::vector<uint64_t> compatabilityHashes;

    /// Dynamic offsets for all descriptor set layouts, ordered by bind order
    std::vector<uint32_t> descriptorDynamicOffsets;

    /// Number of descriptor sets for the user
    uint32_t boundUserDescriptorStates{0};

    /// User push constant length
    uint32_t userPushConstantLength{0};

#if PRMT_METHOD == PRMT_METHOD_UB_PC
    /// PRMT push constant offset
    uint32_t prmtPushConstantOffset{0};
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC

    /// Data push constant offset
    uint32_t dataPushConstantOffset{0};
    uint32_t dataPushConstantLength{0};

    /// Combined push constant mask
    VkShaderStageFlags pushConstantRangeMask{0};

    /// Unique identifier, unique for the type
    uint64_t uid;
};
