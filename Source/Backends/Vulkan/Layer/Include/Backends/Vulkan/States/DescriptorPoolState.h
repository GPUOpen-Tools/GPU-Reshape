#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>

// Std
#include <cstdint>
#include <vector>

// Forward declarations
struct DeviceDispatchTable;
struct DescriptorSetState;

struct DescriptorPoolState {
    /// Backwards reference
    DeviceDispatchTable* table;

    /// User pipeline
    VkDescriptorPool object{VK_NULL_HANDLE};

    /// All descriptor sets
    std::vector<DescriptorSetState*> states;
    
    /// Unique identifier, unique for the type
    uint64_t uid;
};
