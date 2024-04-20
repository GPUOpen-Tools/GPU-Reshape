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

#include <catch2/catch.hpp>

// Backend
#include <Backend/IL/Devices/CPUEmitter.h>
#include <Backend/Resource/TexelAddressAllocator.h>
#include <Backend/Resource/TexelAddressEmitter.h>

template<typename T>
void Test(T token, bool requiresAllResident = true) {
    IL::CPUEmitter emitter;

    Backend::IL::TexelAddressEmitter address(emitter, token);

    ResourceInfo info;
    info.token.width = token.GetWidth();
    info.token.height = token.GetHeight();
    info.token.depthOrSliceCount = token.GetDepthOrSliceCount();
    info.token.mipCount = token.GetMipCount();
    info.token.viewBaseWidth = token.GetViewBaseWidth();
    info.token.viewWidth = token.GetViewWidth();
    info.token.viewBaseMip = token.GetViewBaseMip();
    info.token.viewBaseSlice = token.GetViewBaseSlice();
    info.token.viewSliceCount = token.GetViewSliceCount();
    info.token.viewMipCount = token.GetViewMipCount();
    info.isVolumetric = true;
    
    TexelAddressAllocator allocator;
    uint64_t size = allocator.GetAllocationSize(info);

    std::vector<bool> states;
    states.resize(size, false);

    if (info.isVolumetric) {
        for (uint32_t mip = 0; mip < token.GetMipCount(); mip++) {
            for (uint32_t z = 0; z < token.GetDepthOrSliceCount() >> mip; z++) {
                for (uint32_t y = 0; y < token.GetHeight() >> mip; y++) {
                    for (uint32_t x = 0; x < token.GetWidth() >> mip; x++) {
                        uint32_t offset = address.LocalTextureTexelAddress(x, y, z, mip, true).texelOffset;

                        REQUIRE(!states.at(offset));
                        states.at(offset) = true;
                    }
                }
            }
        }
    } else {
        for (uint32_t z = 0; z < token.GetDepthOrSliceCount(); z++) {
            for (uint32_t mip = 0; mip < token.GetMipCount(); mip++) {
                for (uint32_t y = 0; y < token.GetHeight() >> mip; y++) {
                    for (uint32_t x = 0; x < token.GetWidth() >> mip; x++) {
                        uint32_t offset = address.LocalTextureTexelAddress(x, y, z, mip, false).texelOffset;
                    
                        REQUIRE(!states.at(offset));
                        states.at(offset) = true;
                    }
                }   
            }
        }
    }

    if (requiresAllResident) {
        for (uint64_t i = 0; i < size; i++) {
            REQUIRE(states.at(i));
        }
    }
}

TEST_CASE("Backend.IL.TexelAddressing.Sliced") {
    class TokenEmitter {
    public:
        uint32_t GetWidth() { return 64; }
        uint32_t GetHeight() { return 128; }
        uint32_t GetDepthOrSliceCount() { return 16; }
        uint32_t GetMipCount() { return 3; }
        uint32_t GetViewBaseWidth() { return 0; }
        uint32_t GetViewWidth() { return 64; }
        uint32_t GetViewBaseMip() { return 0; }
        uint32_t GetViewBaseSlice() { return 0; }
        uint32_t GetViewSliceCount() { return 1; }
        uint32_t GetViewMipCount() { return 1; }
        bool IsVolumetric() { return false; }
    };

    Test(TokenEmitter());
}

TEST_CASE("Backend.IL.TexelAddressing.Volumetric") {
    class TokenEmitter {
    public:
        uint32_t GetWidth() { return 64; }
        uint32_t GetHeight() { return 128; }
        uint32_t GetDepthOrSliceCount() { return 16; }
        uint32_t GetMipCount() { return 3; }
        uint32_t GetViewBaseWidth() { return 0; }
        uint32_t GetViewWidth() { return 64; }
        uint32_t GetViewBaseMip() { return 0; }
        uint32_t GetViewBaseSlice() { return 0; }
        uint32_t GetViewSliceCount() { return 1; }
        uint32_t GetViewMipCount() { return 1; }
        bool IsVolumetric() { return true; }
    };

    Test(TokenEmitter());
}

TEST_CASE("Backend.IL.TexelAddressing.Sliced.1x1Mip") {
    class TokenEmitter {
    public:
        uint32_t GetWidth() { return 64; }
        uint32_t GetHeight() { return 165; }
        uint32_t GetDepthOrSliceCount() { return 16; }
        uint32_t GetMipCount() { return 7; }
        uint32_t GetViewBaseWidth() { return 0; }
        uint32_t GetViewWidth() { return 64; }
        uint32_t GetViewBaseMip() { return 0; }
        uint32_t GetViewBaseSlice() { return 0; }
        uint32_t GetViewSliceCount() { return 1; }
        uint32_t GetViewMipCount() { return 1; }
        bool IsVolumetric() { return false; }
    };

    Test(TokenEmitter(), false);
}

TEST_CASE("Backend.IL.TexelAddressing.Volumetric.1x1Mip") {
    class TokenEmitter {
    public:
        uint32_t GetWidth() { return 64; }
        uint32_t GetHeight() { return 128; }
        uint32_t GetDepthOrSliceCount() { return 16; }
        uint32_t GetMipCount() { return 7; }
        uint32_t GetViewBaseWidth() { return 0; }
        uint32_t GetViewWidth() { return 64; }
        uint32_t GetViewBaseMip() { return 0; }
        uint32_t GetViewBaseSlice() { return 0; }
        uint32_t GetViewSliceCount() { return 1; }
        uint32_t GetViewMipCount() { return 1; }
        bool IsVolumetric() { return true; }
    };

    Test(TokenEmitter(), false);
}

TEST_CASE("Backend.IL.TexelAddressing.1D") {
    class TokenEmitter {
    public:
        uint32_t GetWidth() { return 64; }
        uint32_t GetHeight() { return 1; }
        uint32_t GetDepthOrSliceCount() { return 1; }
        uint32_t GetMipCount() { return 1; }
        uint32_t GetViewBaseWidth() { return 0; }
        uint32_t GetViewWidth() { return 64; }
        uint32_t GetViewBaseMip() { return 0; }
        uint32_t GetViewBaseSlice() { return 0; }
        uint32_t GetViewSliceCount() { return 1; }
        uint32_t GetViewMipCount() { return 1; }
        bool IsVolumetric() { return false; }
    };

    Test(TokenEmitter());
}

