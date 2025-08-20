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

struct DescriptorDataHeader {
    /// PRM offsets always start at base, without exception
    /// This also avoids a dependent scalar lookup
    /// uint32_t prmOffset = sizeof(DescriptorDataHeader);

    /// Execution info offset
    uint32_t executionDWordOffset = 0;
};

/// Number of dwords per header
static constexpr uint32_t DescriptorDataHeaderDWordCount = sizeof(DescriptorDataHeader) / sizeof(uint32_t);

struct DescriptorDataControl {
    /// Get the root dword offset
    static uint32_t GetRootDWordOffset(uint32_t rootDWord) {
        return DescriptorDataHeaderDWordCount + rootDWord;
    }
    
    /// Shader visible header
    DescriptorDataHeader header;

    /// Total number of dwords
    uint32_t dwordCount = 0;
};
