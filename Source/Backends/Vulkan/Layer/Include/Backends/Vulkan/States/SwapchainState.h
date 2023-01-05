#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>

// Std
#include <cstdint>
#include <vector>

// Forward declarations
struct DeviceDispatchTable;
struct ImageState;

struct SwapchainState {
    /// Backwards reference
    DeviceDispatchTable* table;

    /// User Sampler
    VkSwapchainKHR object{VK_NULL_HANDLE};

    /// Backbuffer images
    std::vector<ImageState*> imageStates;

    /// Unique identifier, unique for the type
    uint64_t uid;
};
