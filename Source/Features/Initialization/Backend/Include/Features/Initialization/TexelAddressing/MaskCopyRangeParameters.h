﻿// 
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

struct MaskCopyRangeParameters {
    /// Memory offsets aligned to 32
    uint32_t sourceMemoryBaseElementAlign32{0};
    uint32_t destMemoryBaseElementAlign32{0};

    /// Source offsets
    uint32_t sourceBaseX{0};
    uint32_t sourceBaseY{0};
    uint32_t sourceBaseZ{0};

    /// Destination offsets
    uint32_t destBaseX{0};
    uint32_t destBaseY{0};
    uint32_t destBaseZ{0};

    /// Mip offsets
    uint32_t sourceMip{0};
    uint32_t destMip{0};

    /// Dimensions
    uint32_t width{1};
    uint32_t height{1};
    uint32_t depth{1};

    /// Dispatch offset
    uint32_t dispatchOffset{0};

    /// Dispatch width
    uint32_t dispatchWidth{0};

    /// Placement strides, only used in asymmetric copies
    uint32_t placementRowLength{0};
    uint32_t placementImageHeight{0};
};
