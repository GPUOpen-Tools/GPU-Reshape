#pragma once

// Std
#include <cstdint>

struct SpvRecord {
    /// Get the op code
    SpvOp GetOp() const {
        return static_cast<SpvOp>(lowWordCountHighOpCode & SpvOpCodeMask);
    }

    /// Get the total number of words
    uint32_t GetWordCount() const {
        return (lowWordCountHighOpCode >> SpvWordCountShift) & SpvOpCodeMask;
    }

    /// Get the number of operands
    uint32_t GetOperandCount() const {
        return GetWordCount() - 1u;
    }

    /// Deprecate this instruction for writing
    void Deprecate() {
        lowWordCountHighOpCode = 0;
    }

    /// Is this instruction deprecated?
    bool IsDeprecated() const {
        return lowWordCountHighOpCode == 0u;
    }

    /// Header
    uint32_t lowWordCountHighOpCode{0};

    /// Decoupled operands
    const uint32_t* operands{nullptr};
};
