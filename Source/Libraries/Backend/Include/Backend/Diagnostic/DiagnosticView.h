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
#include "DiagnosticMessage.h"

// Common
#include <Common/Assert.h>

template<typename T>
struct DiagnosticView {
    /// Constructor, provides an argument view into a message
    DiagnosticView(const DiagnosticMessage<T>& message) : base(static_cast<const uint8_t*>(message.argumentBase)) {
        end = base + message.argumentSize;
    }

    /// Get an argument
    /// \tparam U type must be exactly the same
    /// \return argument reference
    template<typename U>
    const U& Get() {
        const U* ptr = reinterpret_cast<const U*>(base);
        base += sizeof(U);
        ASSERT(base <= end, "Out of bounds view");
        return *ptr;
    }

private:
    /// Argument bounds
    const uint8_t* base;
    const uint8_t* end;
};
