// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

#include "TypeMap.h"
#include "Format.h"

namespace Backend::IL {
    inline const Type* GetSampledFormatType(TypeMap& map, Format format) {
        switch (format) {
            default:
                ASSERT(false, "Invalid shader buffer info format");
                return nullptr;
            case Format::RGBA32Float:
            case Format::RGBA16Float:
            case Format::RGBA8:
            case Format::RGBA8Snorm:
            case Format::RGBA16:
            case Format::RGBA16Snorm:
            case Format::RGB10A2:
            case Format::RGB10a2UInt:
                return map.FindTypeOrAdd(VectorType {
                    .containedType = map.FindTypeOrAdd(IL::FPType {.bitWidth = 32}),
                    .dimension = 4
                });
            case Format::RGBA32Int:
            case Format::RGBA16Int:
            case Format::RGBA8Int:
                return map.FindTypeOrAdd(VectorType {
                    .containedType = map.FindTypeOrAdd(IL::IntType {.bitWidth = 32, .signedness = true}),
                    .dimension = 4
                });
            case Format::RGBA32UInt:
            case Format::RGBA16UInt:
            case Format::RGBA8UInt:
                return map.FindTypeOrAdd(VectorType {
                    .containedType = map.FindTypeOrAdd(IL::IntType {.bitWidth = 32, .signedness = false}),
                    .dimension = 4
                });
            case Format::RG32Float:
            case Format::RG16Float:
            case Format::RG16:
            case Format::RG8:
            case Format::RG16Snorm:
            case Format::RG8Snorm:
                return map.FindTypeOrAdd(VectorType {
                    .containedType = map.FindTypeOrAdd(IL::FPType {.bitWidth = 32}),
                    .dimension = 2
                });
            case Format::RG32Int:
            case Format::RG16Int:
            case Format::RG8Int:
                return map.FindTypeOrAdd(VectorType {
                    .containedType = map.FindTypeOrAdd(IL::IntType {.bitWidth = 32, .signedness = true}),
                    .dimension = 2
                });
            case Format::RG32UInt:
            case Format::RG16UInt:
            case Format::RG8UInt:
                return map.FindTypeOrAdd(VectorType {
                    .containedType = map.FindTypeOrAdd(IL::IntType {.bitWidth = 32, .signedness = false}),
                    .dimension = 2
                });
            case Format::R11G11B10Float:
                return map.FindTypeOrAdd(VectorType {
                    .containedType = map.FindTypeOrAdd(IL::FPType {.bitWidth = 32}),
                    .dimension = 3
                });
            case Format::R32Float:
            case Format::R32Snorm:
            case Format::R32Unorm:
            case Format::R16Float:
            case Format::R16:
            case Format::R8:
            case Format::R16Snorm:
            case Format::R16Unorm:
            case Format::R8Snorm:
                return map.FindTypeOrAdd(IL::FPType {.bitWidth = 32});
            case Format::R32Int:
            case Format::R16Int:
            case Format::R8Int:
            case Format::R32UInt:
            case Format::R16UInt:
            case Format::R8UInt:
                return map.FindTypeOrAdd(IL::IntType {.bitWidth = 32, .signedness = false});
        }
    }
}
