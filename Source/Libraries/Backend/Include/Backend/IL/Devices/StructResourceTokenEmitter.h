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
#include <Backend/IL/ShaderStruct.h>
#include <Backend/Resource/ResourceToken.h>

namespace IL {
    template<typename T>
    struct StructResourceTokenEmitter {
        StructResourceTokenEmitter(Emitter<T>& emitter, const ShaderStruct<ResourceToken>& info) : emitter(emitter), info(info) {
            
        }

        /// Get the resource physical UID
        IL::ID GetPUID() {
            return info.Get<&ResourceToken::puid>(emitter);
        }

        /// Get the resource type
        uint32_t GetType() {
            return info.Get<&ResourceToken::type>(emitter);
        }

        /// Get the resource width
        uint32_t GetWidth() {
            return info.Get<&ResourceToken::width>(emitter);
        }

        /// Get the resource height
        uint32_t GetHeight() {
            return info.Get<&ResourceToken::height>(emitter);
        }

        /// Get the resource depth or slice count
        uint32_t GetDepthOrSliceCount() {
            return info.Get<&ResourceToken::depthOrSliceCount>(emitter);
        }

        /// Get the mip count
        uint32_t GetMipCount() {
            return info.Get<&ResourceToken::mipCount>(emitter);
        }

        /// Get the mip offset
        uint32_t GetBaseMip() {
            return info.Get<&ResourceToken::baseMip>(emitter);
        }

        /// Get the slice offset
        uint32_t GetBaseSlice() {
            return info.Get<&ResourceToken::baseSlice>(emitter);
        }

    private:
        /// Underlying emitter
        Emitter<T>& emitter;

        /// Shader struct to fetch from
        ShaderStruct<ResourceToken> info;
    };
}
