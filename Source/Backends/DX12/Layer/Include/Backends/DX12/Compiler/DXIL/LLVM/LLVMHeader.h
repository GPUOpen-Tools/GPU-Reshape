#pragma once

#include <cstdint>

enum class LLVMReservedAbbreviation : uint8_t {
    EndBlock,
    EnterSubBlock,
    DefineAbbreviation,
    UnabbreviatedRecord,
};

enum class LLVMBlockInfoRecord : uint8_t {
    SetBID = 1,
    BlockName,
    SetRecordName
};

enum class LLVMAbbreviationEncoding : uint8_t {
    Literal,
    Fixed,
    VBR,
    Array,
    Char6,
    Blob
};

struct LLVMAbbreviationParameter {
    /// Encoding used
    LLVMAbbreviationEncoding encoding{};

    /// Optional value associated with encoding
    uint64_t value{0};
};
