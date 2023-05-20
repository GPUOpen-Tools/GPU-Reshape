#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/States/FenceState.h>

// Common
#include <Common/Containers/ObjectPool.h>

// Std
#include <cstdint>
#include <vector>
#include <mutex>

// Forward declarations
struct ShaderExportQueueState;
struct DeviceDispatchTable;

struct QueueState {
    ~QueueState();

    VkCommandBuffer PopCommandBuffer();

    void PushCommandBuffer(VkCommandBuffer commandBuffer);

    /// Backwards reference
    DeviceDispatchTable* table;

    /// User pipeline
    VkQueue object{VK_NULL_HANDLE};

    /// Family index of this queue
    uint32_t familyIndex{~0U};

    /// Shared command pool
    VkCommandPool commandPool{VK_NULL_HANDLE};

    /// Immediate command buffers
    std::vector<VkCommandBuffer> commandBuffers;

    /// Object pools
    ObjectPool<FenceState> pools_fences;

    /// Current export state
    ShaderExportQueueState* exportState{nullptr};

    /// Shared lock
    std::mutex mutex;

    /// Unique identifier, unique for the type
    uint64_t uid;
};
