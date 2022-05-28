#pragma once

// Layer
#include "LLVMRecord.h"

// Common
#include <Common/Assert.h>
#include <Common/CRC.h>

struct LLVMRecordStringView {
    LLVMRecordStringView() = default;

    /// Construct from record at offset
    LLVMRecordStringView(const LLVMRecord &record, uint32_t offset) :
        operands(record.ops + offset),
        operandCount(record.opCount - offset) {
        ASSERT(offset <= record.opCount, "Out of bounds record string view");

        // Hash contents
        ComputeHash();
    }

    /// Length of this string
    uint32_t Length() const {
        return operandCount;
    }

    /// Copy string to destination
    /// \param out must be able to contain the string
    void Copy(char* out) const {
        for (uint32_t i = 0; i < operandCount; i++) {
            out[i] = operands[i];
        }
    }

    /// Copy string to destination
    /// \param out must be able to contain the string, with the null terminator
    void CopyTerminated(char* out) const {
        Copy(out);
        out[operandCount] = '\0';
    }

    /// Check for equality with rhs cstring
    bool operator==(const char* rhs) const {
        for (uint32_t i = 0; i < operandCount; i++) {
            if (!rhs[i] || rhs[i] != operands[i]) {
                return false;
            }
        }

        return true;
    }

    /// Valid?
    operator bool() const {
        return operands != nullptr;
    }

    /// Get the precomputed hash
    uint64_t GetHash() const {
        return hash;
    }

private:
    void ComputeHash() {
        hash = hash ^ 0xFFFFFFFFU;
        for (uint32_t i = 0; i < operandCount; i++) {
            hash = crcdetail::table[static_cast<char>(operands[i]) ^ (hash & 0xFF)] ^ (hash >> 8);
        }
        hash = hash ^ 0xFFFFFFFFU;
    }

private:
    /// Starting operand
    const uint64_t *operands{nullptr};

    /// Number of operands
    uint32_t operandCount{0};

    /// Precomputed hash
    uint64_t hash{0};
};
