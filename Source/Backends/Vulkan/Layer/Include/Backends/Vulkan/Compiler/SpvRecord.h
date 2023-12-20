// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

struct SpvRecord {
    /// Get the op code
    SpvOp GetOp() const {
        return static_cast<SpvOp>(lowWordCountHighOpCode & SpvOpCodeMask);
    }

    /// Get the total number of words
    uint32_t GetWordCount() const {
        return (lowWordCountHighOpCode >> SpvWordCountShift) & SpvOpCodeMask;
    }

    /// Get the number of operands
    uint32_t GetOperandCount() const {
        return GetWordCount() - 1u;
    }

    /// Deprecate this instruction for writing
    void Deprecate() {
        lowWordCountHighOpCode = 0;
    }

    /// Is this instruction deprecated?
    bool IsDeprecated() const {
        return lowWordCountHighOpCode == 0u;
    }

    /// Header
    uint32_t lowWordCountHighOpCode{0};

    /// Decoupled operands
    const uint32_t* operands{nullptr};
};
