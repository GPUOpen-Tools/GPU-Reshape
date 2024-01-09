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
#include "Backends/Vulkan/Vulkan.h"
#include "Backends/Vulkan/States/PipelineLayoutPhysicalMapping.h"
#include "PipelineLayoutBindingInfo.h"

// Common
#include "Common/Containers/ReferenceObject.h"

// Std
#include <cstdint>
#include <vector>

// Forward declarations
struct DeviceDispatchTable;

struct PipelineLayoutState : public ReferenceObject {
    /// Reference counted destructor
    virtual ~PipelineLayoutState();

    /// Backwards reference
    DeviceDispatchTable* table;

    /// User pipeline
    VkPipelineLayout object{VK_NULL_HANDLE};

    /// Has this layout exhausted all its user slots?
    bool exhausted{false};

    /// Physical mapping
    PipelineLayoutPhysicalMapping physicalMapping;

    /// Combined pipeline hash
    uint64_t compatabilityHash{0};

    /// Compatability hashes for all descriptor set layouts, ordered by bind order
    std::vector<uint64_t> compatabilityHashes;

    /// Dynamic offsets for all descriptor set layouts, ordered by bind order
    std::vector<uint32_t> descriptorDynamicOffsets;

    /// Number of descriptor sets for the user
    uint32_t boundUserDescriptorStates{0};

    /// User push constant length
    uint32_t userPushConstantLength{0};

#if PRMT_METHOD == PRMT_METHOD_UB_PC
    /// PRMT push constant offset
    uint32_t prmtPushConstantOffset{0};
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC

    /// Data push constant offset
    uint32_t dataPushConstantOffset{0};
    uint32_t dataPushConstantLength{0};

    /// Combined push constant mask
    VkShaderStageFlags pushConstantRangeMask{0};

    /// Unique identifier, unique for the type
    uint64_t uid;
};
