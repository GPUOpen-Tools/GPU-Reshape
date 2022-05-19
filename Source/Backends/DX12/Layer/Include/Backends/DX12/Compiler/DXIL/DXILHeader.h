#pragma once

// Std
#include <cstdint>

struct DXILHeader {
    uint16_t programVersion;
    uint16_t programType;
    uint32_t dwordCount;
    uint32_t identifier;
    uint32_t version;
    uint32_t codeOffset;
    uint32_t codeSize;
};

enum class DXILAddressSpace {
    Local = 0,
    Device = 1,
    Constant = 2,
    GroupShared = 3
};
