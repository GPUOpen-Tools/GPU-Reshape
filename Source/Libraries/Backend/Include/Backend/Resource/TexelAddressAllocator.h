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
#include <Backend/Resource/ResourceInfo.h>
#include <Backend/Resource/TexelAddressAllocationInfo.h>

// Std
#include <vector>

class TexelAddressAllocator {
public:
    /** todo[init]: incomplete! */
    
    /// Get the total number of entries needed for a resource
    /// \param info given resource information
    /// \param requiresAlignP2 if true, all resource dimensions are aligned to the upper power of two
    /// \return addressing info
    TexelAddressAllocationInfo GetAllocationInfo(const ResourceInfo& info, bool requiresAlignP2) {
        TexelAddressAllocationInfo out;
        out.mipCount = info.token.mipCount;

        // Buffer types have no complex addressing mechanisms
        if (info.token.GetType() == Backend::IL::ResourceTokenType::Buffer) {
            out.texelCount += info.token.width;
        } else {
            // Align all texture dimensions to a power of two
            uint32_t width = requiresAlignP2 ? std::max(std::bit_ceil(info.token.width - 1u), 1u) : info.token.width;
            uint32_t height = requiresAlignP2 ? std::max(std::bit_ceil(info.token.height - 1u), 1u) : info.token.height;
            uint32_t depth = requiresAlignP2 ? std::max(std::bit_ceil(info.token.depthOrSliceCount - 1u), 1u) : info.token.depthOrSliceCount;

            // If volumetric, the major dimension changes
            if (info.isVolumetric) {
                // Aggregate per mip level
                for (uint32_t mipIndex = 0; mipIndex < info.token.mipCount; mipIndex++) {
                    uint32_t mipWidth  = std::max(1u, static_cast<uint32_t>(std::floor(static_cast<float>(width) / (1u << mipIndex))));
                    uint32_t mipHeight = std::max(1u, static_cast<uint32_t>(std::floor(static_cast<float>(height) / (1u << mipIndex))));
                    uint32_t mipDepth = std::max(1u, static_cast<uint32_t>(std::floor(static_cast<float>(depth) / (1u << mipIndex))));
                    
                    // Just add it!
                    out.subresourceOffsets.Add(out.texelCount);
                    out.texelCount += mipWidth * mipHeight * mipDepth;
                }
            } else {
                // Aggregate per mip level
                for (uint32_t sliceIndex = 0; sliceIndex < info.token.depthOrSliceCount; sliceIndex++) {
                    for (uint32_t mipIndex = 0; mipIndex < info.token.mipCount; mipIndex++) {
                        uint32_t mipWidth  = std::max(1u, static_cast<uint32_t>(std::floor(static_cast<float>(width) / (1u << mipIndex))));
                        uint32_t mipHeight = std::max(1u, static_cast<uint32_t>(std::floor(static_cast<float>(height) / (1u << mipIndex))));

                        // If volumetric, depth is affected by the mip level
                        out.subresourceOffsets.Add(out.texelCount);
                        out.texelCount += mipWidth * mipHeight;
                    }
                }
            }
        }

        // OK
        return out;
    }

    /// Allocate from a given length
    /// \param alignment entry alignment
    /// \param length number of entries
    /// \return offset
    uint64_t Allocate(uint64_t alignment, uint64_t length) {
        uint64_t head = GetHead();

        // All allocations are aligned to 32
        // This greatly helps other operations
        head = (head + alignment - 1) & ~(alignment - 1);
        
        Allocation& allocation = allocations.emplace_back();
        allocation.offset = head;
        allocation.length = length;
        return allocation.offset;
    }

    /// Free an allocation
    void Free() {
        // poof!
    }

private:
    /// Get the head allocation offset
    uint64_t GetHead() const {
        if (allocations.empty()) {
            return 0ull;
        }

        // Start from last end
        const Allocation& last = allocations.back();
        return last.offset + last.length;
    }

private:
    struct Allocation {
        uint64_t offset{UINT64_MAX};
        uint64_t length{UINT64_MAX};
        bool destroyed{false};
    };

    /// All allocations
    std::vector<Allocation> allocations;
};
