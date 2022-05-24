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

enum class LLVMFunctionRecord : uint32_t {
    DeclareBlocks = 1,
    InstBinOp = 2,
    InstCast = 3,
    InstGEP = 4,
    InstSelect = 5,
    InstExtractELT = 6,
    InstInsertELT = 7,
    InstShuffleVec = 8,
    InstCmp = 9,
    InstRet = 10,
    InstBr = 11,
    InstSwitch = 12,
    InstInvoke = 13,
    InstUnwind = 14,
    InstUnreachable = 15,
    InstPhi = 16,
    InstMalloc = 17,
    InstFree = 18,
    InstAlloca = 19,
    InstLoad = 20,
    InstStore = 21,
    InstCall = 22,
    InstVaArg = 23,
    InstStore2 = 24,
    InstGetResult = 25,
    InstExtractVal = 26,
    InstInsertVal = 27,
    InstCmp2 = 28,
    InstVSelect = 29,
    InstInBoundsGEP = 30,
    InstIndirectBR = 31,
    DebugLOC = 32,
    DebugLOCAgain = 33,
    InstCall2 = 34,
    DebugLOC2 = 35
};

enum class LLVMBinOp : uint8_t {
    Add = 0,
    Sub = 1,
    Mul = 2,
    UDiv = 3,
    SDiv = 4,
    URem = 5,
    SRem = 6,
    SHL = 7,
    LShR = 8,
    AShR = 9,
    And = 10,
    Or = 11,
    XOr = 12
};

enum class LLVMConstantRecord : uint32_t {
    SetType = 1,
    Null = 2,
    Undef = 3,
    Integer = 4,
    WideInteger = 5,
    Float = 6,
    Aggregate = 7,
    String = 8,
    CString = 9,
    BinOp = 10,
    Cast = 11,
    GEP = 12,
    Select = 13,
    ExtractELT = 14,
    InsertELT = 15,
    ShuffleVec = 16,
    Cmp = 17,
    InlineASM = 18,
    ShuffleVecEX = 19,
    InBoundsGEP = 20,
    BlockAddress = 21
};

enum class LLVMCastOp : uint8_t {
    Trunc = 0,
    ZExt= 1,
    SExt = 2,
    FPToUI = 3,
    FPToSI = 4,
    UIToFP = 5,
    SIToFP = 6,
    FPTrunc = 7,
    FPExt = 8,
    PtrToInt = 9,
    IntToPtr = 10,
    BitCast = 11
};

struct LLVMAbbreviationParameter {
    /// Encoding used
    LLVMAbbreviationEncoding encoding{};

    /// Optional value associated with encoding
    uint64_t value{0};
};
