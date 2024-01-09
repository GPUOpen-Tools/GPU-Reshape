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
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/States/PipelineLayoutPhysicalMapping.h>

// Std
#include <cstdint>

struct ShaderModuleInstrumentationKey {
    auto AsTuple() const {
        return std::make_tuple(featureBitSet, combinedHash);
    }

    bool operator<(const ShaderModuleInstrumentationKey& key) const {
        return AsTuple() < key.AsTuple();
    }

    /// Number of pipeline layout user bound descriptor sets
    uint32_t pipelineLayoutUserSlots{0};

    /// Data push constant offset after the user PC data
    uint32_t pipelineLayoutDataPCOffset{0};

#if PRMT_METHOD == PRMT_METHOD_UB_PC
    /// PRMT push constant offset after the user PC data
    uint32_t pipelineLayoutPRMTPCOffset{0};
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC

    /// Physical mapping
    PipelineLayoutPhysicalMapping* physicalMapping{nullptr};

    /// Final hash
    uint64_t combinedHash{0};

    /// Feature bit set
    uint64_t featureBitSet{0};
};
