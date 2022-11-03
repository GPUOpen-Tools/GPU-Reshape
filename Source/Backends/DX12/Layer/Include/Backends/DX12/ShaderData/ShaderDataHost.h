#pragma once

// Layer
#include <Backends/DX12/Allocation/Allocation.h>

// Backend
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/ShaderData/ShaderDataInfo.h>

// Std
#include <vector>
#include <mutex>

// Forward declarations
struct DeviceState;

class ShaderDataHost final : public IShaderDataHost {
public:
    explicit ShaderDataHost(DeviceState* table);

    /// Install this host
    /// \return success state
    bool Install();

    /// Populate internal descriptors
    /// \param baseDescriptorHandle the base CPU handle
    /// \param stride stride of a descriptor
    void CreateDescriptors(D3D12_CPU_DESCRIPTOR_HANDLE baseDescriptorHandle, uint32_t stride);

    /// Overrides
    ShaderDataID CreateBuffer(const ShaderDataBufferInfo &info) override;
    ShaderDataID CreateEventData(const ShaderDataEventInfo &info) override;
    void Destroy(ShaderDataID rid) override;
    void Enumerate(uint32_t *count, ShaderDataInfo *out, ShaderDataTypeSet mask) override;

private:
    struct ResourceEntry {
        Allocation allocation;
        ShaderDataInfo info;
    };

private:
    /// Parent device
    DeviceState* device;

    /// Shared lock
    std::mutex mutex;

    /// Free indices to be used immediately
    std::vector<ShaderDataID> freeIndices;

    /// All indices, sparsely populated
    std::vector<uint32_t> indices;

    /// Linear resources
    std::vector<ResourceEntry> resources;
};
