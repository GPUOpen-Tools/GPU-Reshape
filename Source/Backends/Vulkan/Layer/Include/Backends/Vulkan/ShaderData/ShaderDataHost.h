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

// Layer
#include <Backends/Vulkan/Allocation/Allocation.h>
#include <Backends/Vulkan/ShaderData/ConstantShaderDataBuffer.h>

// Backend
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/ShaderData/ShaderDataInfo.h>

// Common
#include <Common/ComRef.h>

// Std
#include <vector>
#include <mutex>
#include <map>

// Forward declarations
struct DeviceDispatchTable;
class DeviceAllocator;

class ShaderDataHost final : public IShaderDataHost {
public:
    explicit ShaderDataHost(DeviceDispatchTable* table);
    ~ShaderDataHost();

    /// Install this host
    /// \return success state
    bool Install();

    /// Create all descriptors
    /// \param set set to be bound
    /// \param bindingOffset offset to the binding data, filled linearly from there
    void CreateDescriptors(VkDescriptorSet set, uint32_t bindingOffset);

    /// Create a constant data buffer
    /// \return buffer
    ConstantShaderDataBuffer CreateConstantDataBuffer();

    /// Create an up to date constant mapping table
    /// \return table
    ShaderConstantsRemappingTable CreateConstantMappingTable();

    /// Get the underlying buffer of a resource
    /// \param rid resource id
    /// \return buffer object
    VkBuffer GetResourceBuffer(ShaderDataID rid);

    /// Get the underlying buffer view of a resource
    /// \param rid resource id
    /// \param format the expected format
    /// \return buffer object
    VkBufferView GetResourceBufferView(ShaderDataID rid, VkFormat format);

    /// Get the allocation of a resource
    /// \param rid resource identifier
    /// \return given allocation
    VmaAllocation GetMappingAllocation(ShaderDataMappingID rid);

    /// Get the index of a program binding
    /// @param programID program
    /// @param rid data id of the binding
    /// @return index
    uint32_t GetBindingIndex(ShaderProgramID programID, ShaderDataID rid);
    
    /// Overrides
    ShaderDataID CreateBuffer(const ShaderDataBufferInfo &info, const char* name) override;
    ShaderDataID CreateBufferBinding(const ShaderProgramID& program, const ShaderDataBufferBindingInfo& info) override;
    ShaderDataID CreateEventData(const ShaderDataEventInfo &info) override;
    ShaderDataID CreateDescriptorData(const ShaderDataDescriptorInfo &info) override;
    void *Map(ShaderDataID rid) override;
    void Unmap(ShaderDataID rid, void *mapped) override;
    ShaderDataMappingID CreateMapping(ShaderDataID data, uint64_t tileCount) override;
    void DestroyMapping(ShaderDataMappingID mid) override;
    void FlushMappedRange(ShaderDataID rid, size_t offset, size_t length) override;
    void Destroy(ShaderDataID rid) override;
    void EnumerateShader(uint32_t *count, ShaderDataInfo *out, ShaderDataTypeSet mask) override;
    void EnumerateProgram(ShaderProgramID programID, uint32_t *count, ShaderDataInfo *out, ShaderDataTypeSet mask) override;
    ShaderDataCapabilityTable GetCapabilityTable() override;

private:
    struct ResourceEntry {
        /// Underlying allocation
        Allocation allocation;

        /// Buffer handles
        VkBuffer buffer{VK_NULL_HANDLE};
        VkBufferView view{VK_NULL_HANDLE};

        /// Memory requirements
        VkMemoryRequirements memoryRequirements;

        /// Top information
        ShaderDataInfo info;

        /// Is this a host resource?
        bool isNonDescriptor = false;
    };

    struct MappingEntry {
        /// Underlying allocation
        VmaAllocation allocation;
    };

    struct ProgramEntry {
        /// All program bindings
        std::vector<ShaderDataID> shaderDataIDs;
    };

private:
    /// Shared allocator
    ComRef<DeviceAllocator> deviceAllocator;

private:
    /// Parent device
    DeviceDispatchTable* table;

    /// All capabilities
    ShaderDataCapabilityTable capabilityTable;

    /// Shared lock
    std::mutex mutex;

private:
    /// Free indices to be used immediately
    std::vector<ShaderDataID> freeIndices;

    /// All indices, sparsely populated
    std::vector<uint32_t> indices;

    /// Linear resources
    std::vector<ResourceEntry> resources;

    /// All programs
    std::map<ShaderProgramID, ProgramEntry> programs;

private:
    /// All free mapping indices
    std::vector<ShaderDataMappingID> freeMappingIndices;

    /// All mappings, sparsely laid out
    std::vector<MappingEntry> mappings;
};
