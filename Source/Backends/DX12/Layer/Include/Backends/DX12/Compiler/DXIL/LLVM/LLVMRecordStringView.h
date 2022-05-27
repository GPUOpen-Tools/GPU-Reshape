#pragma once

// Layer
#include "LLVMRecord.h"

// Common
#include <Common/Assert.h>

struct LLVMRecordStringView {
    LLVMRecordStringView() = default;

    /// Construct from record at offset
    LLVMRecordStringView(const LLVMRecord &record, uint32_t offset) :
        operands(record.ops + offset),
        operandCount(record.opCount - offset) {
        ASSERT(offset <= record.opCount, "Out of bounds record string view");
    }

private:
    /// Starting operand
    const uint64_t *operands{nullptr};

    /// Number of operands
    const uint32_t operandCount{0};
};
