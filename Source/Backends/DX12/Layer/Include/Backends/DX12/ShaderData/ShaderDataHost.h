// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

    /// Get the allocation of a resource
    /// \param rid resource identifier
    /// \return given allocation
    Allocation GetResourceAllocation(ShaderDataID rid);

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
