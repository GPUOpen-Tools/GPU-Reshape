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

// Std
#include <cstdint>

struct PipelineLayoutBindingInfo {
    /// Counter descriptor
    uint32_t counterDescriptorOffset{0};

    /// Stream descriptors
    uint32_t streamDescriptorOffset{0};
    uint32_t streamDescriptorCount{0};

    /// PRMT descriptors
    uint32_t prmtDescriptorOffset{0};

    /// Descriptor data descriptors
    uint32_t descriptorDataDescriptorOffset{0};
    uint32_t descriptorDataDescriptorLength{0};

    /// Shader data constants descriptor
    uint32_t shaderDataConstantsDescriptorOffset{0};

    /// Shader data descriptors
    uint32_t shaderDataDescriptorOffset{0};
    uint32_t shaderDataDescriptorCount{0};
};
