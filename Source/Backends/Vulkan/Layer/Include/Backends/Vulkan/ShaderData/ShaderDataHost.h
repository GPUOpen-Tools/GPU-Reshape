#pragma once

// Layer
#include <Backends/Vulkan/Allocation/Allocation.h>

// Backend
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/ShaderData/ShaderDataInfo.h>

// Common
#include <Common/ComRef.h>

// Std
#include <vector>
#include <mutex>

// Forward declarations
struct DeviceDispatchTable;
struct DeviceAllocator;

class ShaderDataHost final : public IShaderDataHost {
public:
    explicit ShaderDataHost(DeviceDispatchTable* table);
    ~ShaderDataHost();

    /// Install this host
    /// \return success state
    bool Install();

    /// Overrides
    ShaderDataID CreateBuffer(const ShaderDataBufferInfo &info) override;
    ShaderDataID CreateEventData(const ShaderDataEventInfo &info) override;
    void Destroy(ShaderDataID rid) override;
    void Enumerate(uint32_t *count, ShaderDataInfo *out, ShaderDataTypeSet mask) override;

private:
    struct ResourceEntry {
        /// Underlying allocation
        Allocation allocation;

        /// Buffer handles
        VkBuffer buffer{VK_NULL_HANDLE};
        VkBufferView view{VK_NULL_HANDLE};

        /// Top information
        ShaderDataInfo info;
    };

private:
    /// Shared allocator
    ComRef<DeviceAllocator> deviceAllocator;

private:
    /// Parent device
    DeviceDispatchTable* table;

    /// Shared lock
    std::mutex mutex;

    /// Free indices to be used immediately
    std::vector<ShaderDataID> freeIndices;

    /// All indices, sparsely populated
    std::vector<uint32_t> indices;

    /// Linear resources
    std::vector<ResourceEntry> resources;
};
