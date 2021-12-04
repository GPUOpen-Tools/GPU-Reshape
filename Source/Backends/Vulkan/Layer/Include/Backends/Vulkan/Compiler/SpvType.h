#pragma once

#include <cstdint>

enum class SpvTypeKind : uint8_t {
    Bool,
    Void,
    Int,
    FP,
    Vector,
    Matrix,
    Compound,
    Pointer,
    Unexposed
};

struct SpvType {
    SpvType(SpvTypeKind kind) : kind(kind) {

    }

    /// Reinterpret this type
    /// \tparam T the target type
    template<typename T>
    const T* As() const {
        ASSERT(kind == T::kKind, "Invalid cast");
        return static_cast<const T*>(this);
    }

    /// Cast this type
    /// \tparam T the target type
    /// \return nullptr is invalid cast
    template<typename T>
    const T* Cast() const {
        if (kind != T::kKind) {
            return nullptr;
        }

        return static_cast<const T*>(this);
    }

    /// Check if this type is valid
    bool Valid() const {
        return id != InvalidSpvId;
    }

    SpvTypeKind kind;
    SpvId id{InvalidSpvId};
};

struct SpvBoolType : public SpvType {
    static constexpr SpvTypeKind kKind = SpvTypeKind::Bool;

    SpvBoolType() : SpvType(kKind) {

    }
};

struct SpvVoidType : public SpvType {
    static constexpr SpvTypeKind kKind = SpvTypeKind::Void;

    SpvVoidType() : SpvType(kKind) {

    }
};

struct SpvIntType : public SpvType {
    static constexpr SpvTypeKind kKind = SpvTypeKind::Int;

    SpvIntType() : SpvType(kKind) {

    }

    uint8_t bitWidth;
    bool    signedness;
};

struct SpvFPType : public SpvType {
    static constexpr SpvTypeKind kKind = SpvTypeKind::Int;

    SpvFPType() : SpvType(kKind) {

    }

    uint8_t bitWidth;
};

struct SpvVectorType : public SpvType {
    static constexpr SpvTypeKind kKind = SpvTypeKind::Int;

    SpvVectorType() : SpvType(kKind) {

    }

    const SpvType* containedType;
    uint8_t dimension;
};

struct SpvMatrixType : public SpvType {
    static constexpr SpvTypeKind kKind = SpvTypeKind::Int;

    SpvMatrixType() : SpvType(kKind) {

    }

    const SpvType* containedType;
    uint8_t rows;
    uint8_t columns;
};

struct SpvPointerType : public SpvType {
    static constexpr SpvTypeKind kKind = SpvTypeKind::Pointer;

    SpvPointerType() : SpvType(kKind) {

    }

    const SpvType* pointee;
};
