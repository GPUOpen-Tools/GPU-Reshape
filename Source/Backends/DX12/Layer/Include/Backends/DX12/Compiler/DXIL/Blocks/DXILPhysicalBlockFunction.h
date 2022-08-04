#pragma once

// Layer
#include <Backends/DX12/Compiler/DXIL/DXILFunctionDeclaration.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordReader.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordStringView.h>
#include "DXILPhysicalBlockSection.h"

// Common
#include <Common/Containers/TrivialStackVector.h>

/// Function block
struct DXILPhysicalBlockFunction : public DXILPhysicalBlockSection {
    using DXILPhysicalBlockSection::DXILPhysicalBlockSection;

    /// Copy this block
    /// \param out destination block
    void CopyTo(DXILPhysicalBlockFunction& out);

public:
    /// Parse a function
    /// \param block source block
    void ParseFunction(struct LLVMBlock *block);

    /// Parse a module function
    /// \param record source record
    void ParseModuleFunction(struct LLVMRecord& record);

    /// Get the declaration associated with an id
    const DXILFunctionDeclaration* GetFunctionDeclaration(uint32_t id);

public:
    /// Compile a function
    /// \param block block
    void CompileFunction(struct LLVMBlock *block);

    /// Compile a module function
    /// \param record record
    void CompileModuleFunction(struct LLVMRecord& record);

    /// Compile a scalar / vector-of-x operation
    /// \param lhs lhs operand
    /// \param rhs rhs operand
    /// \param functor functor(uint32_t lhs, uint32_t rhs)
    template<typename F>
    void CompileSVOX(IL::ID lhs, IL::ID rhs, F&& functor);

    /// Compile a standard intrinsic call
    /// \param decl given declaration
    /// \param opCount argument op count
    /// \param ops operands
    /// \return call record
    LLVMRecord CompileIntrinsicCall(IL::ID result, const DXILFunctionDeclaration* decl, uint32_t opCount, const uint64_t* ops);

public:
    /// Compile a module function
    /// \param record record
    void StitchModuleFunction(struct LLVMRecord& record);

    /// Compile a function
    /// \param block block
    void StitchFunction(struct LLVMBlock *block);

    /// Remap a given record
    /// \param record
    void RemapRecord(struct LLVMRecord& record);

private:
    /// Create an export handle
    /// \param block appended block
    /// \return user id
    uint32_t CreateExportHandle(struct LLVMBlock* block);

private:
    /// Get the resource size intrinsic
    /// \return shared declaration
    const DXILFunctionDeclaration* GetResourceSizeIntrinsic();

    /// Get the bufferStore.f32 intrinsic
    /// \return shared declaration
    const DXILFunctionDeclaration* GetBufferStoreI32Intrinsic();

    /// Get the createHandle intrinsic
    /// \return shared declaration
    const DXILFunctionDeclaration* GetCreateHandleIntrinsic();

    /// Cached intrinsics
    struct Intrinsics {
        uint32_t resourceSize = ~0u;
        uint32_t bufferStoreI32 = ~0u;
        uint32_t createHandle = ~0u;
    } intrinsics;

private:
    /// Does the record have a result?
    bool HasResult(const struct LLVMRecord& record);

    /// Try to parse an intrinsic
    /// \param basicBlock output bb
    /// \param reader record reader
    /// \param anchor instruction anchor
    /// \param called called function index
    /// \param declaration pulled declaration
    /// \return true if recognized intrinsic
    bool TryParseIntrinsic(IL::BasicBlock *basicBlock, uint32_t recordIdx, LLVMRecordReader &reader, uint32_t anchor, uint32_t called, uint32_t result, const DXILFunctionDeclaration *declaration);

private:
    /// Function visitation counters
    uint32_t stitchFunctionIndex{0};

    /// All function declarations
    TrivialStackVector<DXILFunctionDeclaration, 32> functions;

    /// All internally linked declaration indices
    TrivialStackVector<uint32_t, 32> internalLinkedFunctions;
};
