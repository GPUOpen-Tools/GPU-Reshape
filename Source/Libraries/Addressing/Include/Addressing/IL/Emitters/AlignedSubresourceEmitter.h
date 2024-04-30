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

// Addressing
#include <Addressing/IL/PhysicalMipData.h>

// Backend
#include <Backend/IL/Emitters/Emitter.h>
#include <Backend/IL/Emitters/ExtendedEmitter.h>
#include <Backend/IL/Emitters/ResourceTokenEmitter.h>

namespace Backend::IL {
    template<typename E = Emitter<>, typename RTE = ResourceTokenEmitter<E>>
    struct AlignedSubresourceEmitter {
        /// Constructor
        /// \param emitter destination instruction emitter
        /// \param tokenEmitter destination token emitter
        AlignedSubresourceEmitter(E& emitter, RTE& tokenEmitter) : emitter(emitter), tokenEmitter(tokenEmitter) {
            // Cache the aligned dimensions
            // Only matters for texture dimensions
            widthAlignP2 = AlignToPow2Upper(tokenEmitter.GetWidth());
            heightAlignP2 = AlignToPow2Upper(tokenEmitter.GetHeight());
            depthOrSliceCountAlignP2 = AlignToPow2Upper(tokenEmitter.GetDepthOrSliceCount());
            
            if constexpr (E::kDevice == Device::kCPU) {
                emitter.Assert((widthAlignP2 & (widthAlignP2 - 1)) == 0, "Width must be power of two");
                emitter.Assert((heightAlignP2 & (heightAlignP2 - 1)) == 0, "Height must be power of two");
                emitter.Assert((depthOrSliceCountAlignP2 & (depthOrSliceCountAlignP2 - 1)) == 0, "Depth must be power of two");
            }
        }

    public:
        /// Target handle
        template<typename U>
        using Handle = typename E::template Handle<U>;

        /// Standard handles
        using UInt32 = Handle<uint32_t>;

        /// Calculate the offset of a 2d mip
        /// \param slice the slice to get the offset for
        /// \param mip mip to get the offset for
        /// \return offset
        PhysicalMipData<UInt32> SlicedOffset(UInt32 slice, UInt32 mip) {
            PhysicalMipData<UInt32> out;

            // Get the offset from the current slice level (higher dimension than mips if non-volumetric)
            UInt32 base = SliceOffset(widthAlignP2, heightAlignP2, tokenEmitter.GetMipCount(), slice);

            // Calculate mip dimensions
            ExtendedEmitter extended(emitter);
            out.mipWidth  = extended.template Max<UInt32>(emitter.UInt32(1u), emitter.BitShiftRight(widthAlignP2, mip));
            out.mipHeight = extended.template Max<UInt32>(emitter.UInt32(1u), emitter.BitShiftRight(heightAlignP2, mip));
            
            // w*h - mW*mH
            out.offset = emitter.Add(base, MipOffsetFromDifference(emitter.Sub(TexelCount(widthAlignP2, heightAlignP2), TexelCount(out.mipWidth, out.mipHeight)), 2u));

            return out;
        }

        /// Calculate the offset of a 3d mip
        /// @param mip mip to get offset for
        /// @return offset
        PhysicalMipData<UInt32> VolumetricOffset(UInt32 mip) {
            PhysicalMipData<UInt32> out;
            
            // Calculate mip dimensions
            ExtendedEmitter extended(emitter);
            out.mipWidth  = extended.template Max<UInt32>(emitter.UInt32(1u), emitter.BitShiftRight(widthAlignP2, mip));
            out.mipHeight = extended.template Max<UInt32>(emitter.UInt32(1u), emitter.BitShiftRight(heightAlignP2, mip));
            out.mipDepth = extended.template Max<UInt32>(emitter.UInt32(1u), emitter.BitShiftRight(depthOrSliceCountAlignP2, mip));

            // w*h*d - mW*mH*mD
            out.offset = MipOffsetFromDifference(emitter.Sub(TexelCount(widthAlignP2, heightAlignP2, depthOrSliceCountAlignP2), TexelCount(out.mipWidth, out.mipHeight, out.mipDepth)), 3u);

            return out;
        }

    private:
        /// Calculate the offset of a slice
        /// \param width resource width
        /// \param height resource height
        /// \param mipCount resource mip count
        /// \param slice slice to get offset for
        /// \return offset
        UInt32 SliceOffset(UInt32 width, UInt32 height, UInt32 mipCount, UInt32 slice) {
            ExtendedEmitter extended(emitter);
            
            UInt32 mipWidth  = extended.template Max<UInt32>(emitter.UInt32(1u), emitter.BitShiftRight(width, mipCount));
            UInt32 mipHeight = extended.template Max<UInt32>(emitter.UInt32(1u), emitter.BitShiftRight(height, mipCount));

            // Each mip chain has the same size, just multiply it
            UInt32 mipSize = MipOffsetFromDifference(emitter.Sub(TexelCount(width, height), TexelCount(mipWidth, mipHeight)), 2u);
            return emitter.Mul(mipSize, slice);
        }

        /// Calculate the offset of a particular mip
        /// @param difference mip wise size offset (w*h - mW*mH)
        /// @param dimensionality source dimensionality (1, 2, 3)
        /// @return offset
        UInt32 MipOffsetFromDifference(UInt32 difference, uint32_t dimensionality) {
            // s = 2^d
            UInt32 scale = emitter.UInt32(1u << dimensionality);
            UInt32 scaleSub1 = emitter.UInt32((1u << dimensionality) - 1);

            // (difference * s) / (s-1)
            return emitter.Div(emitter.Mul(difference, scale), scaleSub1);
        }

        /// Calculate the number of 2d texels
        UInt32 TexelCount(UInt32 width, UInt32 height) {
            // w*h
            return emitter.Mul(width, height);
        }

        /// Calculate the number of 3d texels
        UInt32 TexelCount(UInt32 width, UInt32 height, UInt32 depth) {
            // w*h*d
            return emitter.Mul(emitter.Mul(width, height), depth);
        }

        /// Align a resource dimension
        UInt32 AlignToPow2Upper(UInt32 x) {
            ExtendedEmitter extended(emitter);

            // 2u << FirstBitHigh(X - 1)
            UInt32 alignedX = emitter.BitShiftLeft(emitter.UInt32(2u), extended.template FirstBitHigh<UInt32>(emitter.Sub(x, emitter.UInt32(1))));

            // Edge case, if the value is 1, return 1
            UInt32 isOne = emitter.Equal(x, emitter.UInt32(1u));
            return emitter.Select(isOne, emitter.UInt32(1u), alignedX);
        }
        
    private:
        /// Current emitter
        E& emitter;

        /// Resource token emitter
        RTE& tokenEmitter;

        /// Cached dimensions
        IL::ID widthAlignP2{InvalidID};
        IL::ID heightAlignP2{InvalidID};
        IL::ID depthOrSliceCountAlignP2{InvalidID};
    };
}
