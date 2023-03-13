#pragma once

// Layer
#include "Spv.h"
#include "SpvRecord.h"
#include "SpvParseContext.h"

// Backend
#include <Backend/IL/ID.h>

struct SpvRecordReader {
    /// Constructor
    SpvRecordReader(const SpvRecord& record) : record(record) {
        // Does this instruction have a result or result type?
        bool hasResult;
        bool hasResultType;
        SpvHasResultAndType(GetOp(), &hasResult, &hasResultType);

        // Read type
        if (hasResultType) {
            type = record.operands[offset++];
        }

        // Read result
        if (hasResult) {
            result = record.operands[offset++];
        }
    }

    /// Get the op code
    SpvOp GetOp() const {
        return record.GetOp();
    }

    /// Get the number of words
    uint32_t GetWordCount() const {
        return record.GetWordCount();
    }

    /// Get the number of operands
    uint32_t GetOperandCount() const {
        return record.GetOperandCount();
    }

    /// Does the instruction have a result?
    bool HasResult() const {
        return result != IL::InvalidID;
    }

    /// Does the instruction have a result type?
    bool HasResultType() const {
        return type != IL::InvalidID;
    }

    /// Get the result type, must have a result type
    IL::ID GetResultType() const {
        ASSERT(HasResultType(), "Instruction has no result");
        return type;
    }

    /// Get the result, must have a result
    IL::ID GetResult() const {
        ASSERT(HasResult(), "Instruction has no result");
        return result;
    }

    bool HasPendingWords() const {
        return offset < GetOperandCount();
    }

    uint32_t operator++(int) {
        ASSERT(offset < GetWordCount(), "Out of bounds operand");
        return record.operands[offset++];
    }

private:
    const SpvRecord& record;

    /// Optional, instruction type
    IL::ID type{IL::InvalidID};

    /// Optional, instruction id
    IL::ID result{IL::InvalidID};

    /// Current operand offset
    uint32_t offset{0};
};
