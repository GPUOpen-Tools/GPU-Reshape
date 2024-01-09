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

// Common
#include <Common/Containers/TrivialStackVector.h>

/// Stack bit field, bit field with default stack container, and optional heap fallback
struct StackBitField {
    /// Initialize with size
    StackBitField(size_t count) {
        bitfield.Resize((count + 32 - 1) / 32);
        Clear();
    }

    /// Clear all bits
    void Clear() {
        std::memset(bitfield.Data(), 0x0u, bitfield.Size() * sizeof(uint32_t));
    }

    /// Set a bit
    void Set(uint32_t i) {
        GetElement(i) |= 1u << (i % 32);
    }

    /// Clear a bit
    void Clear(uint32_t i) {
        GetElement(i) &= ~(1u << (i % 32));
    }

    /// Get a bit state
    bool Get(uint32_t i) const {
        return GetElement(i) & (1u << (i % 32));
    }

private:
    /// Get the element for a given bit
    uint32_t& GetElement(uint32_t i) {
        return bitfield[i / 32];
    }

    /// Get the element for a given bit
    uint32_t GetElement(uint32_t i) const {
        return bitfield[i / 32];
    }

private:
    TrivialStackVector<uint32_t, 128> bitfield;
};