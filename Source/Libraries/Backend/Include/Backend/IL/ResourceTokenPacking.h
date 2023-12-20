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

// Std
#include <cstdint>

namespace IL {
    /// Token physical unique id, limit of 4'194'304 resources (22 bits)
    static constexpr uint32_t kResourceTokenPUIDShift = 0u;
    static constexpr uint32_t kResourceTokenPUIDBitCount = 22u;
    static constexpr uint32_t kResourceTokenPUIDMask  = (1u << kResourceTokenPUIDBitCount) - 1u;

    /// Unmapped and invalid physical unique ids
    static constexpr uint32_t kResourceTokenPUIDInvalidUndefined     = kResourceTokenPUIDMask - 0;
    static constexpr uint32_t kResourceTokenPUIDInvalidOutOfBounds   = kResourceTokenPUIDMask - 1;
    static constexpr uint32_t kResourceTokenPUIDInvalidTableNotBound = kResourceTokenPUIDMask - 2;
    static constexpr uint32_t kResourceTokenPUIDInvalidStart         = kResourceTokenPUIDInvalidTableNotBound;

    /// Reserved physical unique ids
    static constexpr uint32_t kResourceTokenPUIDReservedNullTexture = 0;
    static constexpr uint32_t kResourceTokenPUIDReservedNullBuffer  = 1;
    static constexpr uint32_t kResourceTokenPUIDReservedNullCBuffer = 2;
    static constexpr uint32_t kResourceTokenPUIDReservedNullSampler = 3;
    static constexpr uint32_t kResourceTokenPUIDReservedCount       = 4;

    /// Token type, texture, buffer, cbuffer or sampler (2 bits)
    static constexpr uint32_t kResourceTokenTypeShift = 22u;
    static constexpr uint32_t kResourceTokenTypeBitCount = 2u;
    static constexpr uint32_t kResourceTokenTypeMask  = (1u << kResourceTokenTypeBitCount) - 1u;

    /// Token sub-resource base, base subresource offset into the designated resource (8 bits)
    static constexpr uint32_t kResourceTokenSRBShift = 24u;
    static constexpr uint32_t kResourceTokenSRBBitCount = 8u;
    static constexpr uint32_t kResourceTokenSRBMask  = (1u << kResourceTokenSRBBitCount) - 1u;
}
