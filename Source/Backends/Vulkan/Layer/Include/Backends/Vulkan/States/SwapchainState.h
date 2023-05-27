#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>

// Std
#include <cstdint>
#include <vector>
#include <chrono>

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

    /// Last present time
    std::chrono::time_point<std::chrono::high_resolution_clock> lastPresentTime = std::chrono::high_resolution_clock::now();

    /// Unique identifier, unique for the type
    uint64_t uid;
};
