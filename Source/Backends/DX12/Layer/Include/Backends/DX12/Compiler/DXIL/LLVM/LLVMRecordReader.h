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
#include "LLVMRecord.h"

struct LLVMRecordReader {
    LLVMRecordReader(const LLVMRecord& record) : record(record) {
        /* */
    }

    /// Consume an operand
    uint64_t ConsumeOp() {
        return record.Op(offset++);
    }

    /// Consume an operand
    uint32_t ConsumeOp32() {
        return static_cast<uint32_t>(ConsumeOp());
    }

    /// Consume an operand
    template<typename T>
    T ConsumeOpAs() {
        return record.OpAs<T>(offset++);
    }

    /// Consume an operand or return a default value if not present
    uint64_t ConsumeOpDefault(uint64_t _default) {
        return !Any() ? _default : ConsumeOp();
    }

    /// Any operands to consume?
    bool Any() const {
        return offset < record.opCount;
    }

    /// Get remaining operands
    uint32_t Remaining() const {
        return record.opCount - offset;
    }

    /// Get remaining operands
    uint32_t Offset() const {
        return offset;
    }

    /// Underlying record
    const LLVMRecord& record;

private:
    /// Current offset
    uint32_t offset{0};
};
