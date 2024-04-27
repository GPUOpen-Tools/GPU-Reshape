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
#include <Backend/IL/Resource/PhysicalMipData.h>
#include <Backend/Resource/TexelAddressAllocationInfo.h>

// Std
#include <cmath>

namespace Backend::IL {
    struct CPUSubresourceEmitter {
        /// Constructor
        /// \param resourceInfo resource to get subresource for
        /// \param info resource addressing information
        CPUSubresourceEmitter(const ResourceInfo& resourceInfo, const TexelAddressAllocationInfo& info) : resourceInfo(resourceInfo), info(info) {
            
        }

        /// Get the offset for a slice major format
        /// \param slice target slice
        /// \param mip target mip
        /// \return offset and mip dimensions
        PhysicalMipData<uint32_t> SlicedOffset(uint32_t slice, uint32_t mip) {
            PhysicalMipData<uint32_t> out;
            out.offset = static_cast<uint32_t>(info.GetSubresourceOffset(slice, mip));
            out.mipWidth = std::max(1u, static_cast<uint32_t>(std::floor(static_cast<float>(resourceInfo.token.width) / (1u << mip))));
            out.mipHeight = std::max(1u, static_cast<uint32_t>(std::floor(static_cast<float>(resourceInfo.token.height) / (1u << mip))));
            return out;
        }
        
        /// Get the offset for a mip major format (i.e. volumetric)
        /// \param mip target mip
        /// \return offset and mip dimensions
        PhysicalMipData<uint32_t> VolumetricOffset(uint32_t mip) {
            PhysicalMipData<uint32_t> out;
            out.offset = static_cast<uint32_t>(info.GetSubresourceOffset(0, mip));
            out.mipWidth = std::max(1u, static_cast<uint32_t>(std::floor(static_cast<float>(resourceInfo.token.width) / (1u << mip))));
            out.mipHeight = std::max(1u, static_cast<uint32_t>(std::floor(static_cast<float>(resourceInfo.token.height) / (1u << mip))));
            out.mipDepth = std::max(1u, static_cast<uint32_t>(std::floor(static_cast<float>(resourceInfo.token.depthOrSliceCount) / (1u << mip))));
            return out;
        }

    private:
        /// Target resource
        ResourceInfo resourceInfo;

        /// Target address info
        TexelAddressAllocationInfo info;
    };
}
