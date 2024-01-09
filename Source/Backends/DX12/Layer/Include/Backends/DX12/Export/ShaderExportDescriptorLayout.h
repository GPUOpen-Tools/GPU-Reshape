// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#pragma once

#include <cstdint>

#include <Backends/DX12/Layer.h>

// Forward declarations
struct DeviceState;

class ShaderExportDescriptorLayout {
public:
    /// Install the layout
    /// \param device parent device
    void Install(DeviceState *device);

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

    /// Get the shader constants handle
    /// \param base base address
    D3D12_CPU_DESCRIPTOR_HANDLE GetShaderConstants(D3D12_CPU_DESCRIPTOR_HANDLE base) {
        return D3D12_CPU_DESCRIPTOR_HANDLE{.ptr = base.ptr + shaderConstantOffset};
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

    /// Offsets to the shader constants
    uint32_t shaderConstantOffset{0};

    /// Offset to the shader datas
    uint32_t shaderDataOffset{0};
};
