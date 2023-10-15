// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#pragma once

// Common
#include <Common/Assert.h>

// Std
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
    UseList = 18,
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
    InstGEPOld = 4,
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
    InstStoreOld = 21,
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
    DebugLOC2 = 35,
    InstFence = 36,
    InstCompareExchangeOld = 37,
    InstAtomicRW = 38,
    InstResume = 39,
    InstLandingPadOld = 40,
    InstLoadAtomic = 31,
    InstStoreAtomicOld = 42,
    InstGEP = 43,
    InstStore = 44,
    InstStoreAtomic = 45,
    InstCompareExchange = 48,
    InstLandingPad = 47
};

enum class LLVMParameterGroupRecord : uint32_t {
    Entry = 3
};

enum class LLVMParameterRecord : uint32_t {
    Entry = 2
};

enum class LLVMParameterGroupRecordIndex : uint32_t {
    Return = 0,
    FunctionAttribute = ~0u
};

enum class LLVMParameterGroupKind : uint32_t {
    WellKnown = 0,
    WellKnownValue = 1,
    String = 3,
    StringValue = 4
};

enum class LLVMParameterGroupValue : uint32_t {
    None = 0,
    Align = 1,
    AlwaysInline = 2,
    ByVal = 3,
    InlineHint = 4,
    InReg = 5,
    MinSize = 6,
    Naked = 7,
    Nest = 8,
    NoAlias = 9,
    NoBuiltin = 10,
    NoCapture = 11,
    NodeDuplicate = 12,
    NoImplicitFloat = 13,
    NoInline = 14,
    NonLazyBind = 15,
    NoRedZone = 16,
    NoReturn = 17,
    NoUnwind = 18,
    OptSize = 19,
    ReadNone = 20,
    ReadOnly = 21,
    Returned = 22,
    ReturnsTwice = 23,
    SignExt = 24,
    AlignStack = 25,
    SSP = 26,
    SSPReq = 27,
    SSPStrong = 28,
    SRet = 29,
    SanitizeAddress = 30,
    SanitizeThread = 31,
    SanitizeMemory = 32,
    UwTable = 33,
    ZeroExt = 34,
    Builtin = 35,
    Cold = 36,
    OptNone = 37,
    InAlloca = 38,
    NonNull = 39,
    JumpTable = 40,
    Dereferenceable = 41,
    DereferenceableOrNull = 42,
    Convergent = 43,
    SafeStack = 44,
    ArgMemOnly = 45,
    SwiftSelf = 46,
    SwifTerror = 47,
    NoRecurse = 48,
    InaccessibleMemOnly = 49,
    InaccessibleMemOnlyOrArgMemOnly = 50,
    AllocSize = 51,
    WriteOnly = 52,
    Speculatable = 53,
    StrictFP = 54,
    SanitizeHWAddress = 55,
    NoCFCheck = 56,
    OptForFuzzing = 57,
    ShadowCallstack = 58,
    SpeculativeLoadHardening = 59,
    ImmArg = 60,
    WillReturn = 61,
    NoFree = 62,
    NoSync = 63,
    SanitizeMemTag = 64,
    PreAllocated = 65,
    NoMerge = 66,
    NullPointerIsValid = 67,
    NoUndef = 68,
    ByRef = 69,
    MustProgress = 70,
    VScaleRange = 74,
    SwiftAsync = 75,
    NoSanitizeCoverage = 76,
    ElementType = 77,
    DisableSanitizerInstrumentation = 78,
    NoSanitizeBounds = 79
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

/// Returns true if the record type is dependent on a branch
inline bool IsBranchDependent(LLVMFunctionRecord record) {
    switch (record) {
        default:
            return false;
        case LLVMFunctionRecord::InstBr:
        case LLVMFunctionRecord::InstIndirectBR:
        case LLVMFunctionRecord::InstPhi:
        case LLVMFunctionRecord::InstSwitch:
            return true;
    }
}

/// Returns true if the record has a result value
inline bool HasValueAllocation(LLVMFunctionRecord record, uint32_t opCount) {
    switch (record) {
        default: {
            ASSERT(false, "Unexpected LLVM function record");
            return false;
        }

            /* Unsupported functions */
        case LLVMFunctionRecord::InstInvoke:
        case LLVMFunctionRecord::InstUnwind:
        case LLVMFunctionRecord::InstFree:
        case LLVMFunctionRecord::InstVaArg:
        case LLVMFunctionRecord::InstIndirectBR:
        case LLVMFunctionRecord::InstMalloc: {
            ASSERT(false, "Unsupported instruction");
            return false;
        }

        case LLVMFunctionRecord::DeclareBlocks:
            return false;
        case LLVMFunctionRecord::InstBinOp:
            return true;
        case LLVMFunctionRecord::InstCast:
            return true;
        case LLVMFunctionRecord::InstGEP:
            return true;
        case LLVMFunctionRecord::InstSelect:
            return true;
        case LLVMFunctionRecord::InstExtractELT:
            return true;
        case LLVMFunctionRecord::InstInsertELT:
            return true;
        case LLVMFunctionRecord::InstShuffleVec:
            return true;
        case LLVMFunctionRecord::InstCmp:
            return true;
        case LLVMFunctionRecord::InstRet:
            return opCount > 0;
        case LLVMFunctionRecord::InstBr:
            return false;
        case LLVMFunctionRecord::InstSwitch:
            return false;
        case LLVMFunctionRecord::InstUnreachable:
            return false;
        case LLVMFunctionRecord::InstPhi:
            return true;
        case LLVMFunctionRecord::InstAlloca:
            return true;
        case LLVMFunctionRecord::InstLoad:
            return true;
        case LLVMFunctionRecord::InstStore:
        case LLVMFunctionRecord::InstStoreOld:
        case LLVMFunctionRecord::InstStore2:
            return false;
        case LLVMFunctionRecord::InstCall:
        case LLVMFunctionRecord::InstCall2:
            // Handle in call
            return false;
        case LLVMFunctionRecord::InstGetResult:
            return true;
        case LLVMFunctionRecord::InstExtractVal:
            return true;
        case LLVMFunctionRecord::InstInsertVal:
            return true;
        case LLVMFunctionRecord::InstCmp2:
            return true;
        case LLVMFunctionRecord::InstVSelect:
            return true;
        case LLVMFunctionRecord::InstInBoundsGEP:
            return true;
        case LLVMFunctionRecord::InstAtomicRW:
            return true;
        case LLVMFunctionRecord::DebugLOC:
            return false;
        case LLVMFunctionRecord::DebugLOCAgain:
            return false;
        case LLVMFunctionRecord::DebugLOC2:
            return false;
    }
}