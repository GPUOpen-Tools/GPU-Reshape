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

// Std
#include <cstdint>

struct DXBCConversionBlob {
    /// Constructor
    DXBCConversionBlob() = default;

    /// Destructor
    ~DXBCConversionBlob() {
        delete blob;
    }

    /// No copy
    DXBCConversionBlob(const DXBCConversionBlob&) = delete;
    DXBCConversionBlob operator=(const DXBCConversionBlob&) = delete;

    /// Move constructor
    DXBCConversionBlob(DXBCConversionBlob&& other) noexcept : blob(other.blob), length(other.length) {
        other.blob = nullptr;
        other.length = 0;
    }

    /// Move assignment
    DXBCConversionBlob& operator=(DXBCConversionBlob&& other) noexcept {
        blob = other.blob;
        length = other.length;
        blob = nullptr;
        length = 0;
        return *this;
    }

    /// Internal blob data
    uint8_t* blob{nullptr};

    /// Byte length of blob
    uint32_t length{0};
};
