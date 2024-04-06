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

// Backend
#include <Backend/IL/ResourceTokenPacking.h>
#include <Backend/IL/ResourceTokenMetadataField.h>

// Std
#include <cstdint>

struct VirtualResourceMapping {
    union {
        struct {
            /// Physical UID of the resource
            uint32_t puid : IL::kResourceTokenPUIDBitCount;

            /// Type identifier of this resource
            uint32_t type : IL::kResourceTokenTypeBitCount;

            /// Ignored padding
            uint32_t pad : IL::kResourceTokenPaddingBitCount;
        };

        /// Opaque key
        uint32_t packedToken;
    };

    /// Width of this mapping
    uint32_t width{1};
    
    /// Height of this mapping
    uint32_t height{1};
    
    /// Depth or number of slices of this mapping
    uint32_t depthOrSliceCount{1};

    /// Mip count of this mapping
    uint32_t mipCount{1};

    /// Base mip of this mapping
    uint32_t baseMip{0};

    /// Base slice of this mapping
    uint32_t baseSlice{0};
};

/// Validation
static_assert(sizeof(VirtualResourceMapping) == sizeof(uint32_t) * static_cast<uint32_t>(Backend::IL::ResourceTokenMetadataField::Count), "Metadata mismatch");
