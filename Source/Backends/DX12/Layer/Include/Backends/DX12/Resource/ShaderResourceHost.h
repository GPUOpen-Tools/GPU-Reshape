#pragma once

// Layer
#include <Backends/DX12/Allocation/Allocation.h>

// Backend
#include <Backend/Resource/IShaderResourceHost.h>
#include <Backend/Resource/ShaderResourceInfo.h>

// Std
#include <vector>
#include <mutex>

// Forward declarations
struct DeviceState;

class ShaderResourceHost final : public IShaderResourceHost {
public:
    explicit ShaderResourceHost(DeviceState* table);

    /// Install this host
    /// \return success state
    bool Install();

    /// Populate internal descriptors
    /// \param baseDescriptorHandle the base CPU handle
    /// \param stride stride of a descriptor
    void CreateDescriptors(D3D12_CPU_DESCRIPTOR_HANDLE baseDescriptorHandle, uint32_t stride);

    /// Overrides
    ShaderResourceID CreateBuffer(const ShaderBufferInfo &info) override;
    void DestroyBuffer(ShaderResourceID rid) override;
    void Enumerate(uint32_t *count, ShaderResourceInfo *out) override;

private:
    struct ResourceEntry {
        Allocation allocation;
        ShaderResourceInfo info;
    };

private:
    /// Parent device
    DeviceState* device;

    /// Shared lock
    std::mutex mutex;

    /// Free indices to be used immediately
    std::vector<ShaderResourceID> freeIndices;

    /// All indices, sparsely populated
    std::vector<uint32_t> indices;

    /// Linear resources
    std::vector<ResourceEntry> resources;
};
