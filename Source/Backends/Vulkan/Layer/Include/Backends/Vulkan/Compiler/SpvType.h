#pragma once

#include <cstdint>

enum class SpvTypeKind : uint8_t {
    Int,
    FP,
    Vector,
    Matrix,
    Compound
};

struct SpvType {
    SpvType(SpvTypeKind kind) : kind(kind) {

    }

    SpvTypeKind kind;
    SpvId id{InvalidSpvId};
};

struct SpvIntType : public SpvType {
    SpvIntType() : SpvType(SpvTypeKind::Int) {

    }

    uint8_t bitWidth;
    bool    signedness;
};

struct SpvFPType : public SpvType {
    SpvFPType() : SpvType(SpvTypeKind::FP) {

    }

    uint8_t bitWidth;
};

struct SpvVectorType : public SpvType {
    SpvVectorType() : SpvType(SpvTypeKind::Vector) {

    }

    SpvType* containedType;
    uint8_t  dimension;
};

struct SpvMatrixType : public SpvType {
    SpvMatrixType() : SpvType(SpvTypeKind::Matrix) {

    }

    SpvType* containedType;
    uint8_t  rows;
    uint8_t  columns;
};
