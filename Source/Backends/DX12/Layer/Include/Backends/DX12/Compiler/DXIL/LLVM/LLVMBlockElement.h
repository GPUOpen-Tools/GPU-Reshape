#pragma once

// Std
#include <cstdint>

/// Given type
enum class LLVMBlockElementType : uint8_t {
    Abbreviation = 0,
    Record = 1,
    Block = 2
};

struct LLVMBlockElement {
    LLVMBlockElement() : type(0), id(0) {

    }

    /// Construct from type and id
    LLVMBlockElement(LLVMBlockElementType type, uint32_t id) : type(static_cast<uint32_t>(type)), id(id) {

    }

    /// Is this element of a type?
    bool Is(LLVMBlockElementType elementType) const {
        return type == static_cast<uint32_t>(elementType);
    }

    uint32_t type : 2;
    uint32_t id : 30;
};
