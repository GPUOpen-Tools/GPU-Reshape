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

// Std
#include <cstdint>

/// Source association
struct SpvSourceAssociation {
    operator bool() const {
        return fileUID != UINT16_MAX;
    }

    /// Get the sorting key
    uint64_t GetKey() const {
        uint64_t key = 0;
        key |= static_cast<uint64_t>(fileUID) << 0ull;
        key |= static_cast<uint64_t>(line) << 16ull;
        key |= static_cast<uint64_t>(column) << 48ull;
        return key;
    }

    uint16_t fileUID{UINT16_MAX};
    uint32_t line{0};
    uint16_t column{0};
};

/// Instruction association
struct SpvInstructionAssociation {
    operator bool() const {
        return functionId != UINT32_MAX;
    }

    uint32_t functionId{UINT32_MAX};
    uint32_t basicBlockId{UINT32_MAX};
    uint32_t instructionIndex{UINT32_MAX};
    uint32_t codeOffset{0};
};
