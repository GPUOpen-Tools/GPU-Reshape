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
    Info = 0,
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

enum class LLVMSymTabRecord : uint8_t {
    Entry = 1,
    BasicBlockEntry = 2,
    FunctionEntry = 3,
    CombinedEntry = 5
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
    BlockAddress = 21,
    Data = 22
};

enum class LLVMCastOp : uint8_t {
    Trunc = 0,
    ZExt = 1,
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

enum class LLVMLinkage : uint8_t {
    ExternalLinkage = 0,
    AvailableExternallyLinkage,
    LinkOnceAnyLinkage,
    LinkOnceODRLinkage,
    WeakAnyLinkage,
    WeakODRLinkage,
    AppendingLinkage,
    InternalLinkage,
    PrivateLinkage,
    ExternalWeakLinkage,
    CommonLinkage
};

enum class LLVMCmpOp : uint8_t {
    FloatFalse = 0,
    FloatOrderedEqual = 1,
    FloatOrderedGreaterThan = 2,
    FloatOrderedGreaterEqual = 3,
    FloatOrderedLessThan = 4,
    FloatOrderedLessEqual = 5,
    FloatOrderedNotEqual = 6,
    FloatOrdered = 7,
    FloatNotOrdered = 8,
    FloatUnorderedEqual = 9,
    FloatUnorderedGreaterThan = 10,
    FloatUnorderedGreaterEqual = 11,
    FloatUnorderedLessThan = 12,
    FloatUnorderedLessEqual = 13,
    FloatUnorderedNotEqual = 14,
    FloatTrue = 15,
    FirstFloatPredecate = FloatFalse,
    LastFloatPredecate = FloatTrue,
    BadFloatPredecate = FloatTrue + 1,
    IntEqual = 32,
    IntNotEqual = 33,
    IntUnsignedGreaterThan = 34,
    IntUnsignedGreaterEqual = 35,
    IntUnsignedLessThan = 36,
    IntUnsignedLessEqual = 37,
    IntSignedGreaterThan = 38,
    IntSignedGreaterEqual = 39,
    IntSignedLessThan = 40,
    IntSignedLessEqual = 41,
    IntFirstPredecate = IntEqual,
    IntLastPredecate = IntSignedLessEqual,
    IntBadPredecate = IntSignedLessEqual + 1
};

enum class LLVMCallingConvention : uint32_t {
    C = 0,
    Fast = 8,
    Cold = 9,
    GHC = 10,
    HiPE = 11,
    WebKit_JS = 12,
    AnyReg = 13,
    PreserveMost = 14,
    PreserveAll = 15,
    Swift = 16,
    CXX_FAST_TLS = 17,
    Tail = 18,
    CFGuard_Check = 19,
    SwiftTail = 20,
    FirstTargetCC = 64,
    X86_StdCall = 64,
    X86_FastCall = 65,
    ARM_APCS = 66,
    ARM_AAPCS = 67,
    ARM_AAPCS_VFP = 68,
    MSP430_INTR = 69,
    X86_ThisCall = 70,
    PTX_Kernel = 71,
    PTX_Device = 72,
    SPIR_FUNC = 75,
    SPIR_KERNEL = 76,
    Intel_OCL_BI = 77,
    X86_64_SysV = 78,
    Win64 = 79,
    X86_VectorCall = 80,
    HHVM = 81,
    HHVM_C = 82,
    X86_INTR = 83,
    AVR_INTR = 84,
    AVR_SIGNAL = 85,
    AVR_BUILTIN = 86,
    AMDGPU_VS = 87,
    AMDGPU_GS = 88,
    AMDGPU_PS = 89,
    AMDGPU_CS = 90,
    AMDGPU_KERNEL = 91,
    X86_RegCall = 92,
    AMDGPU_HS = 93,
    MSP430_BUILTIN = 94,
    AMDGPU_LS = 95,
    AMDGPU_ES = 96,
    AArch64_VectorCall = 97,
    AArch64_SVE_VectorCall = 98,
    WASM_EmscriptenInvoke = 99,
    AMDGPU_Gfx = 100,
    M68k_INTR = 101,
    MaxID = 1023
};

enum class LLVMMetadataRecord : uint8_t {
    StringOld = 1,
    Value = 2,
    Node = 3,
    Name = 4,
    DistinctNode = 5,
    Kind = 6,
    Location = 7,
    OldNode = 8,
    OldFnNode = 9,
    NamedNode = 10,
    Attachment = 11,
    GenericDebug = 12,
    SubRange = 13,
    Enumerator = 14,
    BasicType = 15,
    File = 16,
    DerivedType = 17,
    CompositeType = 18,
    SubroutineType = 19,
    CompileUnit = 20,
    SubProgram = 21,
    LexicalBlock = 22,
    LexicalBlockFile = 23,
    Namespace = 24,
    TemplateType = 25,
    TemplateValue = 26,
    GlobalVar = 27,
    LocalVar = 28,
    Expression = 29,
    ObjProperty = 30,
    ImportedEntity = 31,
    Module = 32,
    Macro = 33,
    MacroFile = 34,
    Strings = 35,
    GlobalDeclAttachment = 36,
    GlobalVarExport = 37,
    IndexOffset = 38,
    Index = 39,
    Label = 40,
    StringType = 41,
    CommonBlock = 44,
    GenericSubRange = 45,
    ArgList = 46
};

struct LLVMAbbreviationParameter {
    /// Encoding used
    LLVMAbbreviationEncoding encoding{};

    /// Optional value associated with encoding
    uint64_t value{0};
};
