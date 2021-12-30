#pragma once

// Layer
#include "Backends/Vulkan/Vulkan.h"

// Common
#include "Common/ReferenceObject.h"

// Std
#include <cstdint>

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

    /// Number of descriptor sets for the user
    uint32_t boundUserDescriptorStates{0};

    /// Unique identifier, unique for the type
    uint64_t uid;
};
