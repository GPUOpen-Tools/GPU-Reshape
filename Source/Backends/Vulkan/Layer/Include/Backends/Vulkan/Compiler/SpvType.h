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
    Array,
    Image,
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

    uint8_t bitWidth{32};
    bool    signedness{false};
};

struct SpvFPType : public SpvType {
    static constexpr SpvTypeKind kKind = SpvTypeKind::Int;

    SpvFPType() : SpvType(kKind) {

    }

    uint8_t bitWidth{32};
};

struct SpvVectorType : public SpvType {
    static constexpr SpvTypeKind kKind = SpvTypeKind::Int;

    SpvVectorType() : SpvType(kKind) {

    }

    const SpvType* containedType{nullptr};
    uint8_t dimension{1};
};

struct SpvMatrixType : public SpvType {
    static constexpr SpvTypeKind kKind = SpvTypeKind::Int;

    SpvMatrixType() : SpvType(kKind) {

    }

    const SpvType* containedType{nullptr};
    uint8_t rows{1};
    uint8_t columns{1};
};

struct SpvPointerType : public SpvType {
    static constexpr SpvTypeKind kKind = SpvTypeKind::Pointer;

    SpvPointerType() : SpvType(kKind) {

    }

    const SpvType* pointee{nullptr};
    SpvStorageClass storageClass{SpvStorageClassGeneric};
};

struct SpvArrayType : public SpvType {
    static constexpr SpvTypeKind kKind = SpvTypeKind::Array;

    SpvArrayType() : SpvType(kKind) {

    }

    auto SortKey() const {
        return std::make_tuple(elementType, count);
    }

    const SpvType* elementType{nullptr};
    uint32_t count{0};
};

struct SpvImageType : public SpvType {
    static constexpr SpvTypeKind kKind = SpvTypeKind::Image;

    SpvImageType() : SpvType(kKind) {

    }

    auto SortKey() const {
        return std::make_tuple(sampledType->id, dimension, depth, arrayed, multisampled, sampled, format);
    }

    const SpvType* sampledType{nullptr};
    SpvDim dimension{SpvDim::SpvDim1D};
    uint32_t depth{1};
    uint32_t arrayed{0};
    uint32_t multisampled{0};
    uint32_t sampled{0};
    SpvImageFormat format{SpvImageFormatR32i};
};

using SpvArraySortKeyType = decltype(std::declval<SpvArrayType>().SortKey());
using SpvImageSortKeyType = decltype(std::declval<SpvImageType>().SortKey());
