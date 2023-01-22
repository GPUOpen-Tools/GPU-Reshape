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

    /// Read the next word within the instruction bounds
    /// \return the consumed word
    uint32_t operator++(int) {
        ASSERT(instructionOffset < Get()->GetWordCount(), "Reading beyond instruction bounds");
        return *(code + (instructionOffset++));
    }

    /// Does the current instruction have any pending words?
    bool HasPendingWords() const {
        return instructionOffset < Get()->GetWordCount();
    }

    /// Get the current instruction code
    /// \return word pointer
    const uint32_t* GetInstructionCode() const {
        return code + instructionOffset;
    }

    /// Get the templating source for the current offset
    IL::Source Source() const {
        return IL::Source::Code(static_cast<uint32_t>(code - source.programBegin));
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
