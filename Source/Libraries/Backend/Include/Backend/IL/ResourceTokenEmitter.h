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
#include "ID.h"
#include "ResourceTokenMetadataField.h"
#include "ResourceTokenPacking.h"

namespace IL {
    template<typename E>
    struct ResourceTokenEmitter {
        ResourceTokenEmitter(E& emitter, ::IL::ID resourceID) : emitter(emitter) {
            token = emitter.ResourceToken(resourceID);

            // Default none initialized
            std::fill_n(dwords, std::size(dwords), IL::InvalidID);
        }

        /// Get the resource physical UID
        ::IL::ID GetPUID() {
            if (puid != IL::InvalidID) {
                return puid;
            }

            IL::ID dword = GetFieldDWord(Backend::IL::ResourceTokenMetadataField::PackedToken);
            return puid = emitter.BitAnd(emitter.BitShiftRight(dword, emitter.UInt32(kResourceTokenPUIDShift)), emitter.UInt32(kResourceTokenPUIDMask));
        }

        /// Get the resource type
        ::IL::ID GetType() {
            if (type != IL::InvalidID) {
                return type;
            }

            IL::ID dword = GetFieldDWord(Backend::IL::ResourceTokenMetadataField::PackedToken);
            return type = emitter.BitAnd(emitter.BitShiftRight(dword, emitter.UInt32(kResourceTokenTypeShift)), emitter.UInt32(kResourceTokenTypeMask));
        }

        /// Get the resource type
        ::IL::ID GetFormat() {
            if (format != IL::InvalidID) {
                return format;
            }

            IL::ID dword = GetFieldDWord(Backend::IL::ResourceTokenMetadataField::PackedFormat);
            return format = emitter.BitAnd(dword, emitter.UInt32(0xFFFF));
        }

        /// Get the resource type
        ::IL::ID GetFormatSize() {
            if (formatSize != IL::InvalidID) {
                return formatSize;
            }

            IL::ID dword = GetFieldDWord(Backend::IL::ResourceTokenMetadataField::PackedFormat);
            return formatSize = emitter.BitShiftRight(dword, emitter.UInt32(16));
        }

        /// Get the resource width
        ::IL::ID GetWidth() {
            return GetFieldDWord(Backend::IL::ResourceTokenMetadataField::Width);
        }

        /// Get the resource height
        ::IL::ID GetHeight() {
            return GetFieldDWord(Backend::IL::ResourceTokenMetadataField::Height);
        }

        /// Get the resource depth or slice count
        ::IL::ID GetDepthOrSliceCount() {
            return GetFieldDWord(Backend::IL::ResourceTokenMetadataField::DepthOrSliceCount);
        }

        /// Get the mip count
        ::IL::ID GetMipCount() {
            return GetFieldDWord(Backend::IL::ResourceTokenMetadataField::MipCount);
        }

        /// Get the mip offset
        ::IL::ID GetViewBaseWidth() {
            return GetFieldDWord(Backend::IL::ResourceTokenMetadataField::ViewBaseWidth);
        }

        /// Get the mip offset
        ::IL::ID GetViewWidth() {
            return GetFieldDWord(Backend::IL::ResourceTokenMetadataField::ViewWidth);
        }

        /// Get the mip offset
        ::IL::ID GetViewBaseMip() {
            return GetFieldDWord(Backend::IL::ResourceTokenMetadataField::ViewBaseMip);
        }

        /// Get the slice offset
        ::IL::ID GetViewBaseSlice() {
            return GetFieldDWord(Backend::IL::ResourceTokenMetadataField::ViewBaseSlice);
        }

        /// Get the slice offset
        ::IL::ID GetViewSliceCount() {
            return GetFieldDWord(Backend::IL::ResourceTokenMetadataField::ViewSliceCount);
        }

        /// Get the slice offset
        ::IL::ID GetViewMipCount() {
            return GetFieldDWord(Backend::IL::ResourceTokenMetadataField::ViewMipCount);
        }

        /// Get the token
        ::IL::ID GetPackedToken() {
            return GetFieldDWord(Backend::IL::ResourceTokenMetadataField::PackedToken);
        }

    private:
        /// Get a dword value
        /// @param field field to fetch
        /// @return dword
        IL::ID GetFieldDWord(Backend::IL::ResourceTokenMetadataField field) {
            uint32_t i = static_cast<uint32_t>(field);

            // Cached?
            if (dwords[i] != InvalidID) {
                return dwords[i];
            }

            // Cache extraction
            dwords[i] = emitter.Extract(token, emitter.GetProgram()->GetConstants().UInt(i)->id);
            return dwords[i];
        }

    private:
        /// Underlying token
        ::IL::ID token;

        /// Cached dwords
        ::IL::ID dwords[static_cast<uint32_t>(Backend::IL::ResourceTokenMetadataField::Count)];

        /// Cache
        ::IL::ID puid{IL::InvalidID};
        ::IL::ID format{IL::InvalidID};
        ::IL::ID formatSize{IL::InvalidID};
        ::IL::ID type{IL::InvalidID};

        /// Current emitter
        E& emitter;
    };
}
