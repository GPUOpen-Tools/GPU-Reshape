#pragma once

// Layer
#include "LLVMRecordAbbreviation.h"

// Common
#include <Common/Assert.h>

// Std
#include <cstdint>
#include <memory>

struct LLVMRecord {
    LLVMRecord() : opCount(0), userRecord(0), hasValue(0) {

    }

    template<typename T>
    LLVMRecord(T id) : id(static_cast<uint32_t>(id)), opCount(0), userRecord(0), hasValue(0) {

    }

    /// Check if this record is of reserved id
    template<typename T>
    bool Is(T value) const {
        return value == static_cast<T>(id);
    }

    /// Interpret this record as a reserved type
    template<typename T>
    T As() const {
        return static_cast<T>(id);
    }

    /// Get an operand
    uint64_t Op(uint32_t i) const {
        ASSERT(i < opCount, "Operand out of bounds");
        return ops[i];
    }

    /// Get an operand
    uint32_t Op32(uint32_t i) const {
        return static_cast<uint32_t>(Op(i));
    }

    /// Get an operand
    uint64_t& Op(uint32_t i) {
        ASSERT(i < opCount, "Operand out of bounds");
        return ops[i];
    }

    /// Get an operand, or return a default value
    uint64_t OpOrDefault(uint32_t i, uint64_t _default) const {
        if (i < opCount) {
            return ops[i];
        }
        return _default;
    }

    /// Get an operand, or return a default value
    bool TryOp(uint32_t i, uint64_t& out) const {
        if (i < opCount) {
            out = ops[i];
            return true;
        }
        return false;
    }

    /// Check if the given operand index exists
    bool IsValidOp(uint32_t i) const {
        return i < opCount;
    }

    /// Convert an operand to a type
    template<typename T>
    T OpAs(uint32_t i) const {
        ASSERT(i < opCount, "Operand out of bounds");
        return static_cast<T>(ops[i]);
    }

    /// Convert an operand to a type
    template<typename T>
    T OpBitCast(uint32_t i) const {
        ASSERT(i < opCount, "Operand out of bounds");
        return *reinterpret_cast<const T*>(&ops[i]);
    }

    /// Convert an operand to a type
    template<typename T>
    void OpBitWrite(uint32_t i, const T& value) const {
        static_assert(sizeof(value) <= sizeof(uint64_t));

        ASSERT(i < opCount, "Operand out of bounds");

        ops[i] = 0;
        std::memcpy(&ops[i], &value, sizeof(value));
    }

    /// Fill all operands sequentially to a given array
    /// \param out start starting operand offset
    /// \param out length must be the number of operands from the offset
    template<typename T>
    void FillOperands(T* out, uint32_t start = 0) const {
        for (uint32_t i = start; i < opCount; i++) {
            out[i - start] = static_cast<T>(ops[i]);
        }
    }

    /// Set as a user record
    /// \param hasLinearResult if true, allocates linear llvm value
    /// \param result value index
    void SetUser(bool hasLinearResult, uint32_t _sourceAnchor, uint32_t _result) {
        userRecord = 1;
        hasValue = hasLinearResult;
        sourceAnchor = _sourceAnchor;
        result = _result;
    }

    /// Set as source record
    /// \param hasLinearResult if true, allocates linear llvm value
    /// \param anchor value index or anchor
    void SetSource(bool hasLinearResult, uint32_t _sourceAnchor) {
        userRecord = 0;
        hasValue = hasLinearResult;
        sourceAnchor = _sourceAnchor;
    }

    /// Identifier of this record, may be reserved
    uint32_t id{~0u};

    /// Abbreviation of this record
    LLVMRecordAbbreviation abbreviation;

    /// Number of operands within this record
    uint32_t opCount : 30;

    /// Is this a user generated record?
    uint32_t userRecord : 1;

    /// Contains a linearly allocated LLVM value?
    uint32_t hasValue : 1;

    /// Allocated result for stitching
    ///  ? As the visitation order can change as a result of user manipulation, the source value index is preserved
    uint32_t sourceAnchor = ~0u;

    /// IL result
    uint32_t result = ~0u;

    /// All operands
    uint64_t* ops{nullptr};

    /// Optional blob size associated
    uint64_t blobSize{0};

    /// Blob size, lifetime owned by parent module
    const uint8_t* blob{nullptr};
};
