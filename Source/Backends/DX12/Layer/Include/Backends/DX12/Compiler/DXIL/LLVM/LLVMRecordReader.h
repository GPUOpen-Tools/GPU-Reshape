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

private:
    const LLVMRecord record;

    /// Current offset
    uint32_t offset{0};
};
