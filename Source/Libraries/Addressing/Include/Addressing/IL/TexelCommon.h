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
#include <Backend/IL/Emitters/ExtendedEmitter.h>
#include <Backend/IL/Emitters/Emitter.h>

namespace IL {
    /// Scalarized coordinates
    struct TexelCoordinateScalar {
        IL::ID x{InvalidID};
        IL::ID y{InvalidID};
        IL::ID z{InvalidID};
    };

    /// Convert a linear index to a 3d coordinate
    /// \param emitter target emitter
    /// \param index linear index
    /// \param width total width of the grid
    /// \param height total height of the grid
    /// \param depth total depth of the grid
    /// \return 3d coordinate
    template<typename T>
    static TexelCoordinateScalar TexelIndexTo3D(IL::Emitter<T>& emitter, IL::ID index, IL::ID width, IL::ID height, IL::ID depth) {
        TexelCoordinateScalar out{};
        out.x = emitter.Rem(index, width);
        out.y = emitter.Rem(emitter.Div(index, width), height);
        out.z = emitter.Div(index, emitter.Mul(width, height));
        return out;
    }

    /// Calculate the logical mip dimensions
    /// \param emitter target emitter
    /// \param width top-level dimension width
    /// \param mipLevel the target mip level
    /// \return mip width
    template<typename T>
    static IL::ID GetLogicalMipDimension(IL::Emitter<T>& emitter, IL::ID width, IL::ID mipLevel) {
        ExtendedEmitter extended(emitter);

        // mipWidth = 2^mip
        IL::ID pow2Mip = emitter.IntToFloat32(emitter.BitShiftLeft(emitter.UInt32(1u), mipLevel));

        // logicalWidth = width / mipWidth
        IL::ID mipFloor = extended.template Floor<IL::ID>(emitter.Div(emitter.IntToFloat32(width), pow2Mip));
            
        // max(1, logicalWidth)
        return extended.template Max<IL::ID>(emitter.UInt32(1u), emitter.FloatToUInt32(mipFloor));
    }
}
