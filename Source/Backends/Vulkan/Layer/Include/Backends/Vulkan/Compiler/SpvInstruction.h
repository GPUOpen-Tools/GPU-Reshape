#pragma once

// Layer
#include "Spv.h"

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

    uint32_t lowWordCountHighOpCode;
};

struct SpvTypeIntInstruction : public SpvInstruction {
    SpvTypeIntInstruction() : SpvInstruction(SpvOpTypeInt, sizeof(SpvTypeIntInstruction) / sizeof(uint32_t)) {

    }

    SpvId result;
    uint32_t bitWidth;
    uint32_t signedness;
};

struct SpvTypeFPInstruction : public SpvInstruction {
    SpvTypeFPInstruction() : SpvInstruction(SpvOpTypeFloat, sizeof(SpvTypeFPInstruction) / sizeof(uint32_t)) {

    }

    SpvId result;
    uint32_t bitWidth;
};

struct SpvConstantInstruction : public SpvInstruction {
    SpvConstantInstruction() : SpvInstruction(SpvOpConstant, sizeof(SpvConstantInstruction) / sizeof(uint32_t)) {

    }

    SpvId resultType;
    SpvId result;
    uint32_t value;
};

struct SpvIAddInstruction : public SpvInstruction {
    SpvIAddInstruction() : SpvInstruction(SpvOpIAdd, sizeof(SpvIAddInstruction) / sizeof(uint32_t)) {

    }

    SpvId resultType;
    SpvId result;
    SpvId lhs;
    SpvId rhs;
};

struct SpvFAddInstruction : public SpvInstruction {
    SpvFAddInstruction() : SpvInstruction(SpvOpFAdd, sizeof(SpvFAddInstruction) / sizeof(uint32_t)) {

    }

    SpvId resultType;
    SpvId result;
    SpvId lhs;
    SpvId rhs;
};

struct SpvImageWritePartialInstruction : public SpvInstruction {
    SpvImageWritePartialInstruction() : SpvInstruction(SpvOpImageWrite, sizeof(SpvImageWritePartialInstruction) / sizeof(uint32_t)) {

    }

    SpvId image;
    SpvId coordinate;
    SpvId texel;
};
