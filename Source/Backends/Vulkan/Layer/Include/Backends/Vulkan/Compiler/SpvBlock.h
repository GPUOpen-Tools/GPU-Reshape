// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
#include "SpvRecord.h"
#include "SpvStream.h"

// Std
#include <vector>

struct SpvBlock {
    /// Write records to a stream
    /// \param out destination stream
    void Write(SpvStream& out) {
        // Preallocate
        uint32_t* words = out.AllocateRaw(GetWordCount());

        // Write all records
        for (const SpvRecord& record : records) {
            if (record.IsDeprecated()) {
                continue;
            }
            
            words[0] = record.lowWordCountHighOpCode;
            std::memcpy(&words[1], record.operands, sizeof(uint32_t) * record.GetOperandCount());
            words += record.GetWordCount();
        }
    }

    /// Get the expected word count
    /// \return word count
    uint32_t GetWordCount() {
        uint32_t count = 0;

        // Summarize
        for (const SpvRecord& record : records) {
            if (record.IsDeprecated()) {
                continue;
            }
            
            count += record.GetWordCount();
        }

        return count;
    }

    /// All records
    std::vector<SpvRecord> records;
};
