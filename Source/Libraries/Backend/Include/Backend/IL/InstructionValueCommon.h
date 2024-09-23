﻿// 
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
#include "Emitters/Emitter.h"

namespace IL {
    template<typename T>
    inline ID BitCastToSignedness(::IL::Emitter<T>& emitter, const ID id, bool _signed) {
        TypeMap& typeMap =  emitter.GetProgram()->GetTypeMap();

        // Must be integer
        const auto* type = typeMap.GetType(id)->Cast<IntType>();
        ASSERT(type, "Expected integer type");

        // Cast if needed
        if (type->signedness != _signed) {
            return emitter.BitCast(id, typeMap.FindTypeOrAdd(IntType {.bitWidth = type->bitWidth, .signedness = _signed}));
        } else {
            return id;
        }
    }
    
    template<typename T>
    inline ID BitCastToSigned(::IL::Emitter<T>& emitter, const ID id) {
        return BitCastToSignedness(emitter, id, true);
    }
    
    template<typename T>
    inline ID BitCastToUnsigned(::IL::Emitter<T>& emitter, const ID id) {
        return BitCastToSignedness(emitter, id, false);
    }
}
