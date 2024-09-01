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
#include "SpvPhysicalBlockSource.h"
#include "SpvInstruction.h"

/// Simple parsing context
struct SpvParseContext {
    SpvParseContext(const SpvPhysicalBlockSource& source) : source(source), code(source.code) {
        // Read the first operands
        if (source.code) {
            ReadOperands();
        }
    }

    /// Get the current instruction
    const SpvInstruction* Get() const {
        return reinterpret_cast<const SpvInstruction*>(code);
    }

    /// Accessor
    const SpvInstruction* operator->() const {
        return Get();
    }

    /// Is the context still in a good state?
    bool IsGood() const {
        ASSERT(code <= source.end, "Instruction offset out of bounds");
        return code < source.end;
    }

    /// Implicit good state
    operator bool() const {
        return IsGood();
    }

    /// Read the next instruction
    void Next() {
        code += Get()->GetWordCount();

        // Read the next operands
        ReadOperands();
    }

    /// Peek the next word within the instruction bounds
    /// \param peekOffset offset to apply, must be in bounds
    /// \return the peeked word
    uint32_t Peek(uint32_t peekOffset = 0) {
        ASSERT(instructionOffset + peekOffset < Get()->GetWordCount(), "Reading beyond instruction bounds");
        return *(code + instructionOffset + peekOffset);
    }

    /// Read the next word within the instruction bounds
    /// \return the consumed word
    uint32_t operator++(int) {
        ASSERT(instructionOffset < Get()->GetWordCount(), "Reading beyond instruction bounds");
        return *(code + (instructionOffset++));
    }

    /// Skip a set number of words
    /// \param count number of words to skip
    void Skip(uint32_t count = 1) {
        ASSERT(instructionOffset + count <= Get()->GetWordCount(), "Reading beyond instruction bounds");
        instructionOffset += count;
    }

    /// Does the current instruction have any pending words?
    bool HasPendingWords() const {
        return instructionOffset < Get()->GetWordCount();
    }

    /// Does the current instruction have any pending words?
    uint32_t PendingWords() const {
        return Get()->GetWordCount() - instructionOffset;
    }

    /// Get the current instruction code
    /// \return word pointer
    const uint32_t* GetInstructionCode() const {
        return code + instructionOffset;
    }

    /// Get the templating source for the current offset
    IL::Source Source() const {
        return IL::Source::User(static_cast<uint32_t>(code - source.programBegin));
    }

    /// Get the block source for the current offset
    uint32_t BlockSourceOffset() const {
        return static_cast<uint32_t>(code - source.code);
    }

    /// Does the instruction have a result?
    bool HasResult() const {
        return id != IL::InvalidID;
    }

    /// Does the instruction have a result type?
    bool HasResultType() const {
        return idType != IL::InvalidID;
    }

    /// Get the result type, must have a result type
    IL::ID GetResultType() const {
        ASSERT(HasResultType(), "Instruction has no result");
        return idType;
    }

    /// Get the result, must have a result
    IL::ID GetResult() const {
        ASSERT(HasResult(), "Instruction has no result");
        return id;
    }

private:
    /// Read operands
    void ReadOperands() {
        // Start reading beyond the header
        instructionOffset = 1;

        // Does this instruction have a result or result type?
        bool hasResult;
        bool hasResultType;
        SpvHasResultAndType(Get()->GetOp(), &hasResult, &hasResultType);

        // Result type?
        if (hasResultType) {
            idType = *(code + instructionOffset);
            instructionOffset++;
        } else {
            idType = IL::InvalidID;
        }

        // Result?
        if (hasResult) {
            id = *(code + instructionOffset);
            instructionOffset++;
        } else {
            id = IL::InvalidID;
        }
    }

private:
    /// Optional operands
    IL::ID idType = IL::InvalidID;
    IL::ID id     = IL::InvalidID;

    /// Parent source
    SpvPhysicalBlockSource source;

    /// Reading bounds
    const uint32_t* code{nullptr};
    const uint32_t* end{nullptr};

    /// Current in-instruction offset
    uint32_t instructionOffset{0};
};
