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

    LLVMBlockElement(LLVMBlockElementType type, uint32_t id) : type(static_cast<uint32_t>(type)), id(id) {

    }

    uint32_t type : 2;
    uint32_t id : 30;
};
