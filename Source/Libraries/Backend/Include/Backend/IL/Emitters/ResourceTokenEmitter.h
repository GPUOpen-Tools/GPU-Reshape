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
#include <Backend/IL/ID.h>
#include <Backend/IL/ResourceTokenMetadataField.h>
#include <Backend/IL/ResourceTokenPacking.h>

namespace IL {
    template<typename E>
    struct ResourceTokenEmitter {
        ResourceTokenEmitter(E& emitter, ID resourceID) : emitter(emitter) {
            token = emitter.ResourceToken(resourceID);

            // Default none initialized
            std::fill_n(dwords, std::size(dwords), IL::InvalidID);
        }

        /// Get the resource physical UID
        ID GetPUID() {
            if (puid != IL::InvalidID) {
                return puid;
            }

            IL::ID dword = GetFieldDWord(IL::ResourceTokenMetadataField::PackedToken);
            return puid = emitter.BitAnd(emitter.BitShiftRight(dword, emitter.UInt32(kResourceTokenPUIDShift)), emitter.UInt32(kResourceTokenPUIDMask));
        }

        /// Get the resource type
        ID GetType() {
            if (type != IL::InvalidID) {
                return type;
            }

            IL::ID dword = GetFieldDWord(IL::ResourceTokenMetadataField::PackedToken);
            return type = emitter.BitAnd(emitter.BitShiftRight(dword, emitter.UInt32(kResourceTokenTypeShift)), emitter.UInt32(kResourceTokenTypeMask));
        }

        /// Get the resource type
        ID GetFormat() {
            if (format != IL::InvalidID) {
                return format;
            }

            IL::ID dword = GetFieldDWord(IL::ResourceTokenMetadataField::PackedFormat);
            return format = emitter.BitAnd(dword, emitter.UInt32(0xFFFF));
        }

        /// Get the resource type
        ID GetFormatSize() {
            if (formatSize != IL::InvalidID) {
                return formatSize;
            }

            IL::ID dword = GetFieldDWord(IL::ResourceTokenMetadataField::PackedFormat);
            return formatSize = emitter.BitShiftRight(dword, emitter.UInt32(16));
        }

        /// Get the resource width
        ID GetWidth() {
            return GetFieldDWord(IL::ResourceTokenMetadataField::Width);
        }

        /// Get the resource height
        ID GetHeight() {
            return GetFieldDWord(IL::ResourceTokenMetadataField::Height);
        }

        /// Get the resource depth or slice count
        ID GetDepthOrSliceCount() {
            return GetFieldDWord(IL::ResourceTokenMetadataField::DepthOrSliceCount);
        }

        /// Get the mip count
        ID GetMipCount() {
            return GetFieldDWord(IL::ResourceTokenMetadataField::MipCount);
        }

        /// Get the resource type
        ID GetViewFormat() {
            if (viewFormat != IL::InvalidID) {
                return viewFormat;
            }

            IL::ID dword = GetFieldDWord(IL::ResourceTokenMetadataField::ViewPackedFormat);
            return viewFormat = emitter.BitAnd(dword, emitter.UInt32(0xFFFF));
        }

        /// Get the resource type
        ID GetViewFormatSize() {
            if (viewFormatSize != IL::InvalidID) {
                return viewFormatSize;
            }

            IL::ID dword = GetFieldDWord(IL::ResourceTokenMetadataField::ViewPackedFormat);
            return viewFormatSize = emitter.BitShiftRight(dword, emitter.UInt32(16));
        }

        /// Get the mip offset
        ID GetViewBaseWidth() {
            return GetFieldDWord(IL::ResourceTokenMetadataField::ViewBaseWidth);
        }

        /// Get the mip offset
        ID GetViewWidth() {
            return GetFieldDWord(IL::ResourceTokenMetadataField::ViewWidth);
        }

        /// Get the mip offset
        ID GetViewBaseMip() {
            return GetFieldDWord(IL::ResourceTokenMetadataField::ViewBaseMip);
        }

        /// Get the slice offset
        ID GetViewBaseSlice() {
            return GetFieldDWord(IL::ResourceTokenMetadataField::ViewBaseSlice);
        }

        /// Get the slice offset
        ID GetViewSliceCount() {
            return GetFieldDWord(IL::ResourceTokenMetadataField::ViewSliceCount);
        }

        /// Get the slice offset
        ID GetViewMipCount() {
            return GetFieldDWord(IL::ResourceTokenMetadataField::ViewMipCount);
        }

        /// Get the token
        ID GetPackedToken() {
            return GetFieldDWord(IL::ResourceTokenMetadataField::PackedToken);
        }

    private:
        /// Get a dword value
        /// @param field field to fetch
        /// @return dword
        IL::ID GetFieldDWord(IL::ResourceTokenMetadataField field) {
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
        ID token;

        /// Cached dwords
        ID dwords[static_cast<uint32_t>(IL::ResourceTokenMetadataField::Count)];

        /// Cache
        ID puid{IL::InvalidID};
        ID format{IL::InvalidID};
        ID formatSize{IL::InvalidID};
        ID type{IL::InvalidID};
        ID viewFormat{IL::InvalidID};
        ID viewFormatSize{IL::InvalidID};

        /// Current emitter
        E& emitter;
    };
}
