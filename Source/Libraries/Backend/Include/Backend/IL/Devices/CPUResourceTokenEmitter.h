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
#include <Backend/Resource/ResourceInfo.h>

namespace IL {
    struct CPUResourceTokenEmitter {
        ResourceTokenEmitter(const ResourceInfo& info) : info(info) {
            
        }

        /// Get the resource physical UID
        uint32_t GetPUID() {
            return info.token.puid;
        }

        /// Get the resource type
        uint32_t GetType() {
            return info.token.type;
        }

        /// Get the resource type
        uint32_t GetFormat() {
            return info.token.formatId;
        }

        /// Get the resource type
        uint32_t GetFormatSize() {
            return info.token.formatSize;
        }

        /// Get the resource width
        uint32_t GetWidth() {
            return info.token.width;
        }

        /// Get the resource height
        uint32_t GetHeight() {
            return info.token.height;
        }

        /// Get the resource depth or slice count
        uint32_t GetDepthOrSliceCount() {
            return info.token.depthOrSliceCount;
        }

        /// Get the mip count
        uint32_t GetMipCount() {
            return info.token.mipCount;
        }

        /// Get the mip offset
        uint32_t GetViewBaseWidth() {
            return info.token.viewBaseWidth;
        }

        /// Get the mip offset
        uint32_t GetViewWidth() {
            return info.token.viewWidth;
        }

        /// Get the mip offset
        uint32_t GetViewBaseMip() {
            return info.token.viewBaseMip;
        }

        /// Get the slice offset
        uint32_t GetViewBaseSlice() {
            return info.token.viewBaseSlice;
        }

        /// Get the slice offset
        uint32_t GetViewSliceCount() {
            return info.token.viewSliceCount;
        }

        /// Get the slice offset
        uint32_t GetViewMipCount() {
            return info.token.viewMipCount;
        }

        /// Get the token
        uint32_t GetPackedToken() {
            return static_cast<uint32_t>(info.token.puid | (static_cast<uint32_t>(info.token.type) << kResourceTokenTypeShift));
        }

    private:
        ResourceInfo info;
    };
}
