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
            if (puid != IL::InvalidID) {
                return puid;
            }

            IL::ID dword = info.Get<&ResourceToken::packedToken>(emitter);
            return emitter.BitAnd(emitter.BitShiftRight(dword, emitter.UInt32(kResourceTokenPUIDShift)), emitter.UInt32(kResourceTokenPUIDMask));
        }

        /// Get the resource type
        IL::ID GetType() {
            if (type != IL::InvalidID) {
                return type;
            }

            IL::ID dword = info.Get<&ResourceToken::packedToken>(emitter);
            return type = emitter.BitAnd(emitter.BitShiftRight(dword, emitter.UInt32(kResourceTokenTypeShift)), emitter.UInt32(kResourceTokenTypeMask));
        }

        /// Get the resource type
        ::IL::ID GetFormat() {
            if (format != IL::InvalidID) {
                return format;
            }
            
            IL::ID dword = info.Get<&ResourceToken::packedFormat>(emitter);
            return format = emitter.BitAnd(dword, emitter.UInt32(0xFFFF));
        }

        /// Get the resource type
        ::IL::ID GetFormatSize() {
            if (formatSize != IL::InvalidID) {
                return formatSize;
            }

            IL::ID dword = info.Get<&ResourceToken::packedFormat>(emitter);
            return formatSize = emitter.BitShiftRight(dword, emitter.UInt32(16));
        }

        /// Get the resource width
        IL::ID GetWidth() {
            return info.Get<&ResourceToken::width>(emitter);
        }

        /// Get the resource height
        IL::ID GetHeight() {
            return info.Get<&ResourceToken::height>(emitter);
        }

        /// Get the resource depth or slice count
        IL::ID GetDepthOrSliceCount() {
            return info.Get<&ResourceToken::depthOrSliceCount>(emitter);
        }

        /// Get the mip count
        IL::ID GetMipCount() {
            return info.Get<&ResourceToken::mipCount>(emitter);
        }

        /// Get the resource type
        ::IL::ID GetViewFormat() {
            if (viewFormat != IL::InvalidID) {
                return viewFormat;
            }
            
            IL::ID dword = info.Get<&ResourceToken::viewPackedFormat>(emitter);
            return viewFormat = emitter.BitAnd(dword, emitter.UInt32(0xFFFF));
        }

        /// Get the resource type
        ::IL::ID GetViewFormatSize() {
            if (viewFormatSize != IL::InvalidID) {
                return viewFormatSize;
            }

            IL::ID dword = info.Get<&ResourceToken::viewPackedFormat>(emitter);
            return viewFormatSize = emitter.BitShiftRight(dword, emitter.UInt32(16));
        }

        /// Get the mip offset
        IL::ID GetViewBaseWidth() {
            return info.Get<&ResourceToken::viewBaseWidth>(emitter);
        }

        /// Get the mip offset
        IL::ID GetViewWidth() {
            return info.Get<&ResourceToken::viewWidth>(emitter);
        }

        /// Get the mip offset
        IL::ID GetViewBaseMip() {
            return info.Get<&ResourceToken::viewBaseMip>(emitter);
        }

        /// Get the slice offset
        IL::ID GetViewBaseSlice() {
            return info.Get<&ResourceToken::viewBaseSlice>(emitter);
        }

        /// Get the slice offset
        IL::ID GetViewSliceCount() {
            return info.Get<&ResourceToken::viewSliceCount>(emitter);
        }

        /// Get the slice offset
        IL::ID GetViewMipCount() {
            return info.Get<&ResourceToken::viewMipCount>(emitter);
        }

    private:
        /// Underlying emitter
        Emitter<T>& emitter;
        
        /// Cache
        ::IL::ID puid{IL::InvalidID};
        ::IL::ID format{IL::InvalidID};
        ::IL::ID formatSize{IL::InvalidID};
        ::IL::ID type{IL::InvalidID};
        ::IL::ID viewFormat{IL::InvalidID};
        ::IL::ID viewFormatSize{IL::InvalidID};

        /// Shader struct to fetch from
        ShaderStruct<ResourceToken> info;
    };
}
