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
