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
#include <Addressing/IL/Emitters/AlignedSubresourceEmitter.h>
#include <Addressing/IL/TexelAddress.h>

// Backend
#include <Backend/IL/Emitters/Emitter.h>
#include <Backend/IL/Emitters/ExtendedEmitter.h>
#include <Backend/IL/Emitters/ResourceTokenEmitter.h>

namespace IL {
    template<
        typename E = Emitter<>,
        typename RTE = ResourceTokenEmitter<E>,
        typename SE = AlignedSubresourceEmitter<E>
    >
    struct TexelAddressEmitter {
        static constexpr bool kGuardCoordinates = true;

        /// Constructor
        /// \param emitter destination instruction emitter
        /// \param tokenEmitter destination token emitter
        TexelAddressEmitter(E& emitter, RTE& tokenEmitter, SE& subresourceEmitter) :
            emitter(emitter),
            tokenEmitter(tokenEmitter),
            subresourceEmitter(subresourceEmitter) {
            
        }

    public:
        /// Target handle
        template<typename U>
        using Handle = typename E::template Handle<U>;

        /// Standard handles
        using UInt32 = Handle<uint32_t>;
        using Float = Handle<float>;

        /// Get the texel address of a buffer offset
        /// \param x resource local x coordinate
        /// \param byteOffset the byte offset into the (x) element
        /// \param byteCount the number of bytes being read or written
        /// \return texel offset
        TexelAddress<UInt32> LocalBufferTexelAddress(UInt32 x, UInt32 byteOffset, uint32_t texelCountLiteral) {
            TexelAddress<UInt32> out;
            
            if (kGuardCoordinates) {
                // Default to not out of bounds
                out.isOutOfBounds = emitter.Bool(false);
                
                out.logicalWidth = GuardCoordinate(out, x, tokenEmitter.GetViewWidth());
            }

            // Offset by base width
            x = emitter.Add(x, tokenEmitter.GetViewBaseWidth());

            IL::ID isUntypedFormat     = emitter.Equal(tokenEmitter.GetFormatSize(),     emitter.UInt32(0));
            IL::ID isUntypedViewFormat = emitter.Equal(tokenEmitter.GetViewFormatSize(), emitter.UInt32(0));

            IL::ID expandedTexel;
            {
                // If the format is expanding, calculate the factor
                IL::ID expansionFactor = emitter.Div(tokenEmitter.GetViewFormatSize(), tokenEmitter.GetFormatSize());

                // If the format is untyped, just use the view format width
                expansionFactor = emitter.Select(isUntypedFormat, tokenEmitter.GetViewFormatSize(), expansionFactor);
            
                // Expanded texel
                expandedTexel = emitter.Mul(x, expansionFactor);
            }

            IL::ID contractedTexel;
            {
                // If the format is expanding, calculate the factor
                IL::ID contractionFactor = emitter.Div(tokenEmitter.GetFormatSize(), tokenEmitter.GetViewFormatSize());

                // If the format is untyped, just use the format width
                contractionFactor = emitter.Select(isUntypedViewFormat, tokenEmitter.GetFormatSize(), contractionFactor);
            
                // Contracted texel
                contractedTexel = emitter.Div(x, contractionFactor);
            }

            // Select expansion or contraction
            IL::ID isExpansion = emitter.GreaterThan(tokenEmitter.GetViewFormatSize(), tokenEmitter.GetFormatSize());
            IL::ID sourceOffset = emitter.Select(isExpansion, expandedTexel, contractedTexel);

            // Offset the "coordinate" by the byte offset, said offset granularity is that of the format
            IL::ID formatWidthOr1 = emitter.Select(isUntypedFormat, emitter.UInt32(1), tokenEmitter.GetFormatSize());
            IL::ID formatByteOffset = emitter.Div(byteOffset, formatWidthOr1);
            sourceOffset = emitter.Add(sourceOffset, formatByteOffset);

            // Determine the number of elements
            out.texelCount = emitter.Div(emitter.UInt32(texelCountLiteral), formatWidthOr1);

            // Constants
            IL::ID zero = emitter.UInt32(0);
            IL::ID one = emitter.UInt32(1);
            
            // Just assume the linear index
            out.x = x;
            out.y = zero;
            out.z = zero;
            out.mip = zero;
            out.logicalHeight = one;
            out.logicalDepth = one;
            out.logicalMips = one;
            out.texelOffset = sourceOffset;
            return out;
        }

        /// Get the texel address of a buffer offset
        /// \param x resource local x coordinate
        /// \param byteOffset the byte offset into the (x) element
        /// \param texelCountLiteral the number of bytes being read or written
        /// \return texel offset
        TexelAddress<UInt32> LocalMemoryTexelAddress(UInt32 x, UInt32 byteOffset, uint32_t texelCountLiteral) {
            TexelAddress<UInt32> out;

            // Is the format per-byte?
            IL::ID isUntypedFormat = emitter.Equal(tokenEmitter.GetFormatSize(), emitter.UInt32(0));
            
            // Offset the "coordinate" by the byte offset, said offset granularity is that of the format
            IL::ID formatWidthOr1 = emitter.Select(isUntypedFormat, emitter.UInt32(1), tokenEmitter.GetFormatSize());
            IL::ID formatByteOffset = emitter.Div(byteOffset, formatWidthOr1);
            IL::ID sourceOffset = emitter.Add(x, formatByteOffset);

            // Determine the number of elements
            out.texelCount = emitter.Div(emitter.UInt32(texelCountLiteral), formatWidthOr1);
            
            if (kGuardCoordinates) {
                // Default to not out of bounds
                out.isOutOfBounds = emitter.Bool(false);
                out.logicalWidth = tokenEmitter.GetViewWidth();

                // Guard the offset to width - byteRange, since we're requesting multiple texels
                GuardCoordinate(out, sourceOffset,  emitter.Sub(out.logicalWidth, out.texelCount));
            }

            // Offset by base width
            sourceOffset = emitter.Add(sourceOffset, tokenEmitter.GetViewBaseWidth());

            // Constants
            IL::ID zero = emitter.UInt32(0);
            IL::ID one = emitter.UInt32(1);
            
            // Just assume the linear index
            out.x = x;
            out.y = zero;
            out.z = zero;
            out.mip = zero;
            out.logicalHeight = one;
            out.logicalDepth = one;
            out.logicalMips = one;
            out.texelOffset = sourceOffset;
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
            TexelAddress<UInt32> out;
            
            // Guard mip coordinate
            // Do this before offsetting by the base mip, to save a little bit of ALU
            if (kGuardCoordinates) {
                // Default to not out of bounds
                out.isOutOfBounds = emitter.Bool(false);
                
                out.logicalMips = GuardCoordinate(out, mip, tokenEmitter.GetViewMipCount());
            }

            // Offset by base mip
            mip = emitter.Add(mip, tokenEmitter.GetViewBaseMip());

            // If volumetric, mipping affects depth
            IL::ID texelAddress;
            if (isVolumetric) {
                // Get the offset from the current mip level
                PhysicalMipData<UInt32> mipData = subresourceEmitter.VolumetricOffset(mip);

                // Guard 3d mip coordinates
                if (kGuardCoordinates) {
                    out.logicalWidth = GuardCoordinateToLogicalMip(out, x, tokenEmitter.GetWidth(), mip);
                    out.logicalHeight = GuardCoordinateToLogicalMip(out, y, tokenEmitter.GetHeight(), mip);
                    out.logicalDepth = GuardCoordinateToLogicalMip(out, z, tokenEmitter.GetDepthOrSliceCount(), mip);
                }

                // z * w * h + y * w + x
                UInt32 intraTexelOffset = emitter.Mul(z, emitter.Mul(mipData.mipWidth, mipData.mipHeight));
                intraTexelOffset = emitter.Add(intraTexelOffset, emitter.Mul(y, mipData.mipWidth));
                intraTexelOffset = emitter.Add(intraTexelOffset, x);

                // Actual offset is mip + intra-mip
                texelAddress = emitter.Add(mipData.offset, intraTexelOffset);
            } else {
                // Offset by base slice
                z = emitter.Add(z, tokenEmitter.GetViewBaseSlice());

                // Guard the slice index
                if (kGuardCoordinates) {
                    out.logicalDepth = GuardCoordinate(out, z, tokenEmitter.GetDepthOrSliceCount());
                }

                // Then, offset by the current mip level
                PhysicalMipData<UInt32> mipData = subresourceEmitter.SlicedOffset(z, mip);

                // Guard 2d mip coordinates
                if (kGuardCoordinates) {
                    out.logicalWidth = GuardCoordinateToLogicalMip(out, x, tokenEmitter.GetWidth(), mip);
                    out.logicalHeight = GuardCoordinateToLogicalMip(out, y, tokenEmitter.GetHeight(), mip);
                }

                // y * w + x
                UInt32 intraTexelOffset = emitter.Add(emitter.Mul(y, mipData.mipWidth), x);

                // Actual offset is slice/mip offset + intra-mip
                texelAddress = emitter.Add(mipData.offset, intraTexelOffset);
            }

            // Just assume the linear index
            out.x = x;
            out.y = y;
            out.z = z;
            out.mip = mip;
            out.texelOffset = texelAddress;
            out.texelCount = emitter.UInt32(1u);
            return out;
        }

    private:
        /// Guard a coordinate against its bounds
        /// \param value coordinate to guard
        /// \param width coordinate bound
        UInt32 GuardCoordinate(TexelAddress<UInt32>& address, UInt32& value, UInt32 width) {
            ExtendedEmitter extended(emitter);

            // Out of bounds if value >= Width
            address.isOutOfBounds = emitter.Or(address.isOutOfBounds, emitter.GreaterThanEqual(value, width));

            // Min coordinate against max-1
            value = extended.template Clamp<UInt32>(value, emitter.UInt32(0), emitter.Sub(width, emitter.UInt32(1)));
            return width;
        }

        /// Guard a coordinate against its bounds at a specific mip level
        /// \param value coordinate to guard
        /// \param width coordinate bound
        /// \param mipLevel the target mip level
        UInt32 GuardCoordinateToLogicalMip(TexelAddress<UInt32>& address, UInt32& value, UInt32 width, UInt32 mipLevel) {
            ExtendedEmitter extended(emitter);

            // mipWidth = 2^mip
            Float pow2Mip = emitter.IntToFloat32(emitter.BitShiftLeft(emitter.UInt32(1u), mipLevel));

            // logicalWidth = width / mipWidth
            Float mipFloor = extended.template Floor<Float>(emitter.Div(emitter.IntToFloat32(width), pow2Mip));
            
            // max(1, logicalWidth)
            width = extended.template Max<UInt32>(emitter.UInt32(1u), emitter.FloatToUInt32(mipFloor));

            // Guard against the logical size
            return GuardCoordinate(address, value, width);
        }

    private:        
        /// Current emitter
        E& emitter;

        /// Resource token emitter
        RTE& tokenEmitter;

        /// Subresource emitter
        SE& subresourceEmitter;
    };
}
