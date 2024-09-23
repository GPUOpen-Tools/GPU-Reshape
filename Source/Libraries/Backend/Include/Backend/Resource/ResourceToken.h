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
#include <Backend/IL/ResourceTokenType.h>
#include <Backend/IL/Format.h>

// Std
#include <cstdint>

/// Unpacked token type
struct ResourceToken {
    /// Get the type of the resource
    IL::ResourceTokenType GetType() const {
        return static_cast<IL::ResourceTokenType>(type);
    }

    /// Get the format of the resource
    IL::Format GetFormat() const {
        return static_cast<IL::Format>(formatId);
    }

    /// Default all view properties to the full range
    void DefaultViewToRange() {
        viewPackedFormat = packedFormat;
        viewWidth = width;
        viewSliceCount = depthOrSliceCount;
        viewMipCount = mipCount;
    }

    /// Packed token header
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

    // Packed format header
    union {
        struct {
            /// Format of the resource
            uint32_t formatId : 16;

            /// Size of the format
            uint32_t formatSize : 16;
        };

        /// Opaque key
        uint32_t packedFormat;
    };
    
    /// Width of the resource
    uint32_t width{1};
    
    /// Height of the resource
    uint32_t height{1};
    
    /// Depth or number of slices in the resource
    uint32_t depthOrSliceCount{1};

    /// Mip count in the resource
    uint32_t mipCount{1};

    // Packed view format header
    union {
        struct {
            /// Format of the resource
            uint32_t viewFormatId : 16;

            /// Size of the format
            uint32_t viewFormatSize : 16;
        };

        /// Opaque key
        uint32_t viewPackedFormat;
    };

    /// Base width, i.e., offset of the linear index, of this mapping
    /// Only applies to linearly addressable mappings
    uint32_t viewBaseWidth{0};

    /// Width, i.e., offset of the linear index, of this mapping
    /// Only applies to linearly addressable mappings
    uint32_t viewWidth{0};

    /// Base mip of this mapping
    uint32_t viewBaseMip{0};

    /// Base slice of this mapping
    uint32_t viewBaseSlice{0};

    /// Number of slices in this mapping
    uint32_t viewSliceCount{1};

    /// Number of mips in this mapping
    uint32_t viewMipCount{1};
};
