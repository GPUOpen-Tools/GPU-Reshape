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
#include <Backend/IL/Emitter.h>
#include <Backend/IL/ExtendedEmitter.h>
#include <Backend/IL/ResourceTokenEmitter.h>
#include <Backend/Resource/TexelAddress.h>

namespace Backend::IL {
    template<typename E = Emitter<>, typename RTE = ResourceTokenEmitter<E>>
    struct TexelAddressEmitter {
        static constexpr bool kGuardCoordinates = true;

        /// Constructor
        /// \param emitter destination instruction emitter
        /// \param tokenEmitter destination token emitter
        TexelAddressEmitter(E& emitter, RTE& tokenEmitter) : emitter(emitter), tokenEmitter(tokenEmitter) {
            // Cache the aligned dimensions
            // Only matters for texture dimensions
            widthAlignP2 = AlignToPow2Upper(tokenEmitter.GetWidth());
            heightAlignP2 = AlignToPow2Upper(tokenEmitter.GetHeight());
            depthOrSliceCountAlignP2 = AlignToPow2Upper(tokenEmitter.GetDepthOrSliceCount());
        }

    public:
        /// Target handle
        template<typename U>
        using Handle = typename E::template Handle<U>;

        /// Standard handles
        using UInt32 = Handle<uint32_t>;

        /// Get the texel address of a buffer offset
        /// \param x resource local x coordinate
        /// \return texel offset
        TexelAddress<UInt32> LocalBufferTexelAddress(UInt32 x) {
            if (kGuardCoordinates) {
                ExtendedEmitter extended(emitter);

                // Min all coordinates against max-1
                x = extended.template Clamp<UInt32>(x, emitter.UInt32(0), emitter.Sub(tokenEmitter.GetViewWidth(), emitter.UInt32(1)));
            }

            // Offset by base width
            x = emitter.Add(x, tokenEmitter.GetViewBaseWidth());

            // Just assume the linear index
            TexelAddress<UInt32> out;
            out.x = x;
            out.y = emitter.UInt32(0);
            out.z = emitter.UInt32(0);
            out.mip = emitter.UInt32(0);
            out.texelOffset = x;
            return out;
        }
        
        /// Get the texel address of a 3d offset
        /// \param x resource local x coordinate
        /// \param y resource local y coordinate
        /// \param z resource local z coordinate
        /// \param mip destination mip
        /// \param isVolumetric is this a volumetric (non-sliced) resource, affects offset calculation
        /// \return texel offset
        TexelAddress<UInt32> LocalTextureTexelAddress(UInt32 x, UInt32 y, UInt32 z, UInt32 mip, bool isVolumetric) {
            if (kGuardCoordinates) {
                ExtendedEmitter extended(emitter);

                // Min all coordinates against max-1
                x = extended.template Clamp<UInt32>(x, emitter.UInt32(0), emitter.Sub(tokenEmitter.GetWidth(), emitter.UInt32(1)));
                y = extended.template Clamp<UInt32>(y, emitter.UInt32(0), emitter.Sub(tokenEmitter.GetHeight(), emitter.UInt32(1)));
                z = extended.template Clamp<UInt32>(z, emitter.UInt32(0), emitter.Sub(tokenEmitter.GetDepthOrSliceCount(), emitter.UInt32(1)));
            }

            // Offset by base mip
            mip = emitter.Add(mip, tokenEmitter.GetViewBaseMip());

            // If volumetric, mipping affects depth
            IL::ID texelAddress;
            if (isVolumetric) {
                // Get the offset from the current mip level
                MipData mipData = MipOffset(widthAlignP2, heightAlignP2, depthOrSliceCountAlignP2, mip);

                // z * w * h + y * w + x
                UInt32 intraTexelOffset = emitter.Mul(z, emitter.Mul(mipData.mipWidth, mipData.mipHeight));
                intraTexelOffset = emitter.Add(intraTexelOffset, emitter.Mul(y, mipData.mipWidth));
                intraTexelOffset = emitter.Add(intraTexelOffset, x);

                // Actual offset is mip + intra-mip
                texelAddress = emitter.Add(mipData.offset, intraTexelOffset);
            } else {
                // Offset by base slice
                z = emitter.Add(z, tokenEmitter.GetViewBaseSlice());

                // Get the offset from the current slice level (higher dimension than mips if non-volumetric)
                UInt32 base = SliceOffset(widthAlignP2, heightAlignP2, tokenEmitter.GetMipCount(), z);

                // Then, offset by the current mip level
                MipData mipData = MipOffset(widthAlignP2, heightAlignP2, mip);

                // Slice offset + mip offset
                base = emitter.Add(base, mipData.offset);

                // y * w + x
                UInt32 intraTexelOffset = emitter.Add(emitter.Mul(y, mipData.mipWidth), x);

                // Actual offset is slice/mip offset + intra-mip
                texelAddress = emitter.Add(base, intraTexelOffset);
            }
            

            // Just assume the linear index
            TexelAddress<UInt32> out;
            out.x = x;
            out.y = y;
            out.z = z;
            out.mip = mip;
            out.texelOffset = texelAddress;
            return out;
        }

    private:
        struct MipData {
            UInt32 offset;
            UInt32 mipWidth;
            UInt32 mipHeight;
            UInt32 mipDepth;
        };

        /// Calculate the offset of a slice
        /// \param width resource width
        /// \param height resource height
        /// \param mipCount resource mip count
        /// \param slice slice to get offset for
        /// \return offset
        UInt32 SliceOffset(UInt32 width, UInt32 height, UInt32 mipCount, UInt32 slice) {
            UInt32 mipWidth  = emitter.BitShiftRight(width, mipCount);
            UInt32 mipHeight = emitter.BitShiftRight(height, mipCount);

            // Each mip chain has the same size, just multiply it
            UInt32 mipSize = MipOffsetFromDifference(emitter.Sub(TexelCount(width, height), TexelCount(mipWidth, mipHeight)), 2u);
            return emitter.Mul(mipSize, slice);
        }

        /// Calculate the offset of a 2d mip
        /// \param width resource width
        /// \param height resource height
        /// \param mip mip to get offset for
        /// \return offset
        MipData MipOffset(UInt32 width, UInt32 height, UInt32 mip) {
            if constexpr (E::kDevice == Device::kCPU) {
                emitter.Assert((width & (width - 1)) == 0, "Width must be power of two");
                emitter.Assert((height & (height - 1)) == 0, "Height must be power of two");
            }

            MipData out;
            out.mipWidth  = emitter.BitShiftRight(width, mip);
            out.mipHeight = emitter.BitShiftRight(height, mip);
            
            // w*h - mW*mH
            out.offset = MipOffsetFromDifference(emitter.Sub(TexelCount(width, height), TexelCount(out.mipWidth, out.mipHeight)), 2u);

            return out;
        }

        /// Calculate the offset of a 3d mip
        /// \param width resource width
        /// \param height resource height
        /// @param depth resource depth
        /// @param mip mip to get offset for
        /// @return offset
        MipData MipOffset(UInt32 width, UInt32 height, UInt32 depth, UInt32 mip) {
            if constexpr (E::kDevice == Device::kCPU) {
                emitter.Assert((width & (width - 1)) == 0, "Width must be power of two");
                emitter.Assert((height & (height - 1)) == 0, "Height must be power of two");
                emitter.Assert((depth & (depth - 1)) == 0, "Depth must be power of two");
            }
            
            MipData out;
            out.mipWidth  = emitter.BitShiftRight(width, mip);
            out.mipHeight = emitter.BitShiftRight(height, mip);
            out.mipDepth = emitter.BitShiftRight(depth, mip);

            // w*h*d - mW*mH*mD
            out.offset = MipOffsetFromDifference(emitter.Sub(TexelCount(width, height, depth), TexelCount(out.mipWidth, out.mipHeight, out.mipDepth)), 3u);

            return out;
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
