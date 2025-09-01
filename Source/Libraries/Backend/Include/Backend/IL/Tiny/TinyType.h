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
#include <Backend/IL/TypeKind.h>
#include <Backend/IL/AddressSpace.h>
#include <Backend/IL/TextureDimension.h>
#include <Backend/IL/Format.h>
#include <Backend/IL/ResourceSamplerMode.h>

// Common
#include <Common/Align.h>

// MSVC tight packing
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif // _MSC_VER

namespace Backend::IL::Tiny {
    using RelativeTinyID = uint16_t;

    struct TypeIDArray {
        uint32_t count;
        // RelativeTinyID ids[...];
    };
    
    struct ALIGN_PACK Type {
        TypeKind kind;
    };

    struct ALIGN_PACK IntType : Type {
        uint8_t bitWidth;
        uint8_t signedness;
    };

    struct ALIGN_PACK FPType : Type {
        uint8_t bitWidth;
    };

    struct ALIGN_PACK VectorType : Type {
        RelativeTinyID containedType;
        uint8_t dimension;
    };

    struct ALIGN_PACK MatrixType : Type {
        RelativeTinyID containedType;
        uint8_t rows;
        uint8_t columns;
    };

    struct ALIGN_PACK PointerType : Type {
        RelativeTinyID pointee;
        AddressSpace addressSpace;
    };

    struct ALIGN_PACK ArrayType : Type {
        RelativeTinyID elementType;
        uint32_t count;
    };

    struct ALIGN_PACK TextureType : Type {
        RelativeTinyID sampledType;
        TextureDimension dimension;
        uint8_t multisampled;
        ResourceSamplerMode samplerMode;
        Format format;
    };

    struct ALIGN_PACK BufferType : Type {
        RelativeTinyID elementType;
        ResourceSamplerMode samplerMode;
        Format texelType;
        uint8_t byteAddressing;
    };

    struct ALIGN_PACK FunctionType : Type {
        RelativeTinyID returnType;
        TypeIDArray parameterTypes;
    };

    struct ALIGN_PACK StructType : Type {
        TypeIDArray memberTypes;
    };
}

// MSVC tight packing
#ifdef _MSC_VER
#pragma pack(pop)
#endif // _MSC_VER
