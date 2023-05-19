#pragma once

// Layer
#include <Backends/DX12/Allocation/Allocation.h>
#include <Backends/DX12/ShaderData/ConstantShaderDataBuffer.h>

// Backend
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/ShaderData/ShaderDataInfo.h>

// Common
#include <Common/Allocator/Vector.h>

// Std
#include <vector>
#include <mutex>

// Forward declarations
struct DeviceState;

class ShaderDataHost final : public IShaderDataHost {
public:
    explicit ShaderDataHost(DeviceState* table);
    ~ShaderDataHost();

    /// Install this host
    /// \return success state
    bool Install();

    /// Populate internal descriptors
    /// \param baseDescriptorHandle the base CPU handle
    /// \param stride stride of a descriptor
    void CreateDescriptors(D3D12_CPU_DESCRIPTOR_HANDLE baseDescriptorHandle, uint32_t stride);

    /// Create a constant data buffer
    /// \return buffer
    ConstantShaderDataBuffer CreateConstantDataBuffer();

    /// Create an up to date constant mapping table
    /// \return table
    ShaderConstantsRemappingTable CreateConstantMappingTable();

    /// Overrides
    ShaderDataID CreateBuffer(const ShaderDataBufferInfo &info) override;
    ShaderDataID CreateEventData(const ShaderDataEventInfo &info) override;
    ShaderDataID CreateDescriptorData(const ShaderDataDescriptorInfo &info) override;
    void *Map(ShaderDataID rid) override;
    void FlushMappedRange(ShaderDataID rid, size_t offset, size_t length) override;
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
    Vector<ShaderDataID> freeIndices;

    /// All indices, sparsely populated
    Vector<uint32_t> indices;

    /// Linear resources
    Vector<ResourceEntry> resources;
};
