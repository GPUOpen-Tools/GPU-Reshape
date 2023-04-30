#pragma once

#include <cstdint>

#include <Backends/DX12/Layer.h>

// Forward declarations
struct DeviceState;

class ShaderExportDescriptorLayout {
public:
    /// Install the layout
    /// \param device parent device
    /// \param stride descriptor stride
    void Install(DeviceState *device, uint32_t stride);

    /// Get the export counter handle
    /// \param base base address
    D3D12_CPU_DESCRIPTOR_HANDLE GetExportCounter(D3D12_CPU_DESCRIPTOR_HANDLE base) {
        return D3D12_CPU_DESCRIPTOR_HANDLE{.ptr = base.ptr + shaderExportCounterOffset};
    }

    /// Get the export stream handle
    /// \param base base address
    D3D12_CPU_DESCRIPTOR_HANDLE GetExportStream(D3D12_CPU_DESCRIPTOR_HANDLE base, uint32_t index) {
        return D3D12_CPU_DESCRIPTOR_HANDLE{.ptr = base.ptr + shaderExportStreamOffset + descriptorStride * index};
    }

    /// Get the resource PRMT handle
    /// \param base base address
    D3D12_CPU_DESCRIPTOR_HANDLE GetResourcePRMT(D3D12_CPU_DESCRIPTOR_HANDLE base) {
        return D3D12_CPU_DESCRIPTOR_HANDLE{.ptr = base.ptr + resourcePRMTOffset};
    }

    /// Get the sampler PRMT handle
    /// \param base base address
    D3D12_CPU_DESCRIPTOR_HANDLE GetSamplerPRMT(D3D12_CPU_DESCRIPTOR_HANDLE base) {
        return D3D12_CPU_DESCRIPTOR_HANDLE{.ptr = base.ptr + samplerPRMTOffset};
    }

    /// Get the shader data handle
    /// \param base base address
    D3D12_CPU_DESCRIPTOR_HANDLE GetShaderData(D3D12_CPU_DESCRIPTOR_HANDLE base, uint32_t index) {
        return D3D12_CPU_DESCRIPTOR_HANDLE{.ptr = base.ptr + shaderDataOffset + descriptorStride * index};
    }

    /// Get the number of descriptors
    uint32_t Count() const {
        return descriptorOffset / descriptorStride;
    }

private:
    /// Byte stride of a descriptor
    uint32_t descriptorStride{0};

    /// Base address of a descriptor
    uint32_t descriptorOffset{0};

    /// Offset to the shader export counters
    uint32_t shaderExportCounterOffset{0};

    /// Offset to the shader export streams
    uint32_t shaderExportStreamOffset{0};

    /// Offsets to the PRMT data
    uint32_t resourcePRMTOffset{0};
    uint32_t samplerPRMTOffset{0};

    /// Offset to the shader datas
    uint32_t shaderDataOffset{0};
};
