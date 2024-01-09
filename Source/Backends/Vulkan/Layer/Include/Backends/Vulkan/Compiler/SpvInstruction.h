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

// Layer
#include "Spv.h"

// Std
#include <cstdint>

struct SpvInstruction {
    /// Constructor
    SpvInstruction() : lowWordCountHighOpCode(0) {
    }

    /// Constructor
    SpvInstruction(SpvOp op, uint32_t wordCount) {
        lowWordCountHighOpCode = (op & SpvOpCodeMask) | ((wordCount & SpvOpCodeMask) << SpvWordCountShift);
    }

    /// Get the spirv op code
    SpvOp GetOp() const {
        return static_cast<SpvOp>(lowWordCountHighOpCode & SpvOpCodeMask);
    }

    /// Get the spirv instruction word count, includes the base type
    uint32_t GetWordCount() const {
        return (lowWordCountHighOpCode >> SpvWordCountShift) & SpvOpCodeMask;
    }

    /// Get the instruction ptr
    uint32_t* Ptr() {
        return &lowWordCountHighOpCode;
    }

    /// Get an instruction word
    uint32_t& Word(uint32_t i) {
        return *(Ptr() + i);
    }

    /// Get an instruction word
    uint32_t& operator[](uint32_t i) {
        return Word(i);
    }

    /// Get the instruction ptr
    const uint32_t* Ptr() const {
        return &lowWordCountHighOpCode;
    }

    /// Get an instruction argument
    const uint32_t& Word(uint32_t i) const {
        return *(Ptr() + i);
    }

    /// Get an instruction word
    const uint32_t& operator[](uint32_t i) const {
        return Word(i);
    }

    uint32_t lowWordCountHighOpCode;
};
