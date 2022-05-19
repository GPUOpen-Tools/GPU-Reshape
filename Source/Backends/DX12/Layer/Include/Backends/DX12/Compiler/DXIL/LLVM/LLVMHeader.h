#pragma once

#include <cstdint>

/*
 * LLVM Specification
 *   https://llvm.org/docs/BitCodeFormat.html#module-code-version-record
 */

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

enum class LLVMReservedBlock : uint8_t {
    Module = 8,
    Parameter = 9,
    ParameterGroup = 10,
    Constants = 11,
    Function = 12,
    ValueSymTab = 14,
    Metadata = 15,
    MetadataAttachment = 16,
    Type = 17,
    StrTab = 23
};

enum class LLVMModuleRecord : uint8_t {
    Version = 1,
    Triple = 2,
    DataLayout = 3,
    ASM = 4,
    SectionName = 5,
    DepLib = 6,
    GlobalVar = 7,
    Function = 8,
    Alias = 9,
    GCName = 11
};

enum class LLVMTypeRecord : uint8_t {
    NumEntry = 1,
    Void = 2,
    Half = 10,
    Float = 3,
    Double = 4,
    Label = 5,
    Opaque = 6,
    Integer = 7,
    Pointer = 8,
    Array = 11,
    Vector = 12,
    MetaData = 16,
    StructAnon = 18,
    StructName = 19,
    StructNamed = 20,
    Function = 21,
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
