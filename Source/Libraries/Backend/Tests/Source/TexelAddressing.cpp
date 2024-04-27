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

// Backend
#include <Backend/IL/Devices/CPUResourceTokenEmitter.h>
#include <Backend/IL/Devices/CPUSubresourceEmitter.h>
#include <Backend/IL/Resource/TexelAddressEmitter.h>
#include <Backend/IL/Devices/CPUEmitter.h>
#include <Backend/Resource/TexelAddressAllocator.h>

// Catch
#include <catch2/catch.hpp>

/// Populates all texel addresses and test for their uniqueness
/// \param info resource info
/// \param addressInfo allocation info
/// \param addressEmitter address emitter
/// \param requiresAllResident should all texels be tested for residence?
template<typename S>
void PopulateAndTestUniqueAddressing(const ResourceInfo& info, const TexelAddressAllocationInfo& addressInfo, S& addressEmitter, bool requiresAllResident = true) {
    std::vector<bool> states;
    states.resize(addressInfo.texelCount, false);

    if (info.isVolumetric) {        
        for (uint32_t mip = 0; mip < info.token.mipCount; mip++) {
            uint32_t depth = std::max(1u, static_cast<uint32_t>(std::floor(static_cast<float>(info.token.depthOrSliceCount) / (1u << mip))));
            uint32_t height = std::max(1u, static_cast<uint32_t>(std::floor(static_cast<float>(info.token.height) / (1u << mip))));
            uint32_t width = std::max(1u, static_cast<uint32_t>(std::floor(static_cast<float>(info.token.width) / (1u << mip))));
        
            for (uint32_t z = 0; z < depth; z++) {
                for (uint32_t y = 0; y < height; y++) {
                    for (uint32_t x = 0; x < width; x++) {
                        uint32_t offset = addressEmitter.LocalTextureTexelAddress(x, y, z, mip, true).texelOffset;

                        REQUIRE(!states.at(offset));
                        states.at(offset) = true;
                    }
                }
            }
        }
    } else {
        for (uint32_t z = 0; z < info.token.depthOrSliceCount; z++) {
            for (uint32_t mip = 0; mip < info.token.mipCount; mip++) {
                uint32_t height = std::max(1u, static_cast<uint32_t>(std::floor(static_cast<float>(info.token.height) / (1u << mip))));
                uint32_t width = std::max(1u, static_cast<uint32_t>(std::floor(static_cast<float>(info.token.width) / (1u << mip))));
                
                for (uint32_t y = 0; y < height; y++) {
                    for (uint32_t x = 0; x < width; x++) {
                        uint32_t offset = addressEmitter.LocalTextureTexelAddress(x, y, z, mip, false).texelOffset;
                    
                        REQUIRE(!states.at(offset));
                        states.at(offset) = true;
                    }
                }   
            }
        }
    }

    if (requiresAllResident) {
        for (uint64_t i = 0; i < addressInfo.texelCount; i++) {
            REQUIRE(states.at(i));
        }
    }
}

/// Populates all texel addresses and test for their uniqueness
/// \param info resource info
/// \param lastBlocksAllResident should all texels be tested for residence?
void PopulateAndTestUniqueAddressing(ResourceInfo info, bool lastBlocksAllResident = true) {
    IL::CPUEmitter emitter;

    // Shared allocator
    TexelAddressAllocator allocator;
    
    // Test with precomputed offsets
    {
        // Allocate without padding
        TexelAddressAllocationInfo addressInfo = allocator.GetAllocationInfo(info, false);

        // Setup emitters
        Backend::IL::CPUResourceTokenEmitter tokenEmitter(info);
        Backend::IL::CPUSubresourceEmitter   subresourceEmitter(info, addressInfo);
        Backend::IL::TexelAddressEmitter     address(emitter, tokenEmitter, subresourceEmitter);

        // Packed layout, always resident
        PopulateAndTestUniqueAddressing(info, addressInfo, address, true);
    }

    // Test with runtime inferred offsets
    // Some limitations on dangling blocks
    {
        // Get aligned 2d mip levels
        uint32_t alignedMipCount = std::min(
            static_cast<uint32_t>(std::ceil(std::log2(info.token.width))),
            static_cast<uint32_t>(std::ceil(std::log2(info.token.height)))
        );

        // If volumetric, account for Z too
        if (info.isVolumetric) {
            alignedMipCount = std::min(alignedMipCount, static_cast<uint32_t>(std::ceil(std::log2(info.token.depthOrSliceCount))));
        }

        // Limit to the lowest mip block
        info.token.mipCount = std::min(info.token.mipCount, alignedMipCount);
        info.token.viewMipCount = info.token.mipCount;
    
        // Allocate with padding
        TexelAddressAllocationInfo addressInfo = allocator.GetAllocationInfo(info, true);

        // Setup emitters
        Backend::IL::CPUResourceTokenEmitter   tokenEmitter(info);
        Backend::IL::AlignedSubresourceEmitter subresourceEmitter(emitter, tokenEmitter);
        Backend::IL::TexelAddressEmitter       address(emitter, tokenEmitter, subresourceEmitter);
    
        PopulateAndTestUniqueAddressing(info, addressInfo, address, lastBlocksAllResident);
    }
}

TEST_CASE("Backend.IL.BufferAddressing.1D") {
    ResourceInfo info;
    info.token.width = 64;
    info.token.formatSize = 1;
    info.token.viewFormatSize = 1;

    IL::CPUEmitter                         emitter;
    Backend::IL::CPUResourceTokenEmitter   tokenEmitter(info);
    Backend::IL::AlignedSubresourceEmitter subresourceEmitter(emitter, tokenEmitter);
    Backend::IL::TexelAddressEmitter       address(emitter, tokenEmitter, subresourceEmitter);

    REQUIRE(address.LocalBufferTexelAddress(0).texelOffset == 0);
    REQUIRE(address.LocalBufferTexelAddress(1).texelOffset == 1);
    REQUIRE(address.LocalBufferTexelAddress(2).texelOffset == 2);
    REQUIRE(address.LocalBufferTexelAddress(3).texelOffset == 3);
}

TEST_CASE("Backend.IL.BufferAddressing.1D.ViewExpansion") {
    ResourceInfo info;
    info.token.width = 64;
    info.token.formatSize = 0; // R1
    info.token.viewFormatSize = 4; // R32

    IL::CPUEmitter                         emitter;
    Backend::IL::CPUResourceTokenEmitter   tokenEmitter(info);
    Backend::IL::AlignedSubresourceEmitter subresourceEmitter(emitter, tokenEmitter);
    Backend::IL::TexelAddressEmitter       address(emitter, tokenEmitter, subresourceEmitter);

    for (uint32_t i = 0; i < 64; i++) {
        REQUIRE(address.LocalBufferTexelAddress(i).texelOffset == i * 4);
    }
}

TEST_CASE("Backend.IL.BufferAddressing.1D.ViewContraction") {
    ResourceInfo info;
    info.token.width = 64;
    info.token.formatSize = 4; // R32
    info.token.viewFormatSize = 0; // R1

    IL::CPUEmitter                         emitter;
    Backend::IL::CPUResourceTokenEmitter   tokenEmitter(info);
    Backend::IL::AlignedSubresourceEmitter subresourceEmitter(emitter, tokenEmitter);
    Backend::IL::TexelAddressEmitter       address(emitter, tokenEmitter, subresourceEmitter);

    for (uint32_t i = 0; i < 64; i++) {
        REQUIRE(address.LocalBufferTexelAddress(i).texelOffset == i / 4);
    }
}

TEST_CASE("Backend.IL.TexelAddressing.Sliced") {
    ResourceInfo info;
    info.token.width = 64;
    info.token.height = 128;
    info.token.depthOrSliceCount = 16;
    info.token.mipCount = 3;
    info.token.viewBaseWidth = 0;
    info.token.viewWidth = 64;
    info.token.viewBaseMip = 0;
    info.token.viewBaseSlice = 0;
    info.token.viewSliceCount = 16;
    info.token.viewMipCount = 3;
    info.isVolumetric = false;
    PopulateAndTestUniqueAddressing(info);
}

TEST_CASE("Backend.IL.TexelAddressing.Volumetric") {
    ResourceInfo info;
    info.token.width = 64;
    info.token.height = 128;
    info.token.depthOrSliceCount = 16;
    info.token.mipCount = 3;
    info.token.viewBaseWidth = 0;
    info.token.viewWidth = 64;
    info.token.viewBaseMip = 0;
    info.token.viewBaseSlice = 0;
    info.token.viewSliceCount = 16;
    info.token.viewMipCount = 3;
    info.isVolumetric = true;
    PopulateAndTestUniqueAddressing(info);
}

TEST_CASE("Backend.IL.TexelAddressing.Sliced.1x1Mip") {
    ResourceInfo info;
    info.token.width = 64;
    info.token.height = 165;
    info.token.depthOrSliceCount = 16;
    info.token.mipCount = 7;
    info.token.viewBaseWidth = 0;
    info.token.viewWidth = 64;
    info.token.viewBaseMip = 0;
    info.token.viewBaseSlice = 0;
    info.token.viewSliceCount = 16;
    info.token.viewMipCount = 7;
    info.isVolumetric = false;
    PopulateAndTestUniqueAddressing(info, false);
}

TEST_CASE("Backend.IL.TexelAddressing.Volumetric.1x1Mip") {
    ResourceInfo info;
    info.token.width = 64;
    info.token.height = 128;
    info.token.depthOrSliceCount = 16;
    info.token.mipCount = 7;
    info.token.viewBaseWidth = 0;
    info.token.viewWidth = 64;
    info.token.viewBaseMip = 0;
    info.token.viewBaseSlice = 0;
    info.token.viewSliceCount = 16;
    info.token.viewMipCount = 7;
    info.isVolumetric = true;
    PopulateAndTestUniqueAddressing(info, false);
}

TEST_CASE("Backend.IL.TexelAddressing.1D") {
    ResourceInfo info;
    info.token.width = 64;
    info.token.height = 1;
    info.token.depthOrSliceCount = 1;
    info.token.mipCount = 1;
    info.token.viewBaseWidth = 0;
    info.token.viewWidth = 64;
    info.token.viewBaseMip = 0;
    info.token.viewBaseSlice = 0;
    info.token.viewSliceCount = 1;
    info.token.viewMipCount = 1;
    info.isVolumetric = false;
    PopulateAndTestUniqueAddressing(info);
}
