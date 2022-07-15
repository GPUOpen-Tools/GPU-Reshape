#pragma once

// Layer
#include <Backends/DX12/Compiler/DXIL/DXILFunctionDeclaration.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordReader.h>
#include "DXILPhysicalBlockSection.h"

// Common
#include <Common/Containers/TrivialStackVector.h>

/// Function block
struct DXILPhysicalBlockFunction : public DXILPhysicalBlockSection {
    using DXILPhysicalBlockSection::DXILPhysicalBlockSection;

    /// Parse a function
    /// \param block source block
    void ParseFunction(const struct LLVMBlock *block);

    /// Parse a module function
    /// \param record source record
    void ParseModuleFunction(const struct LLVMRecord& record);

    /// Get the declaration associated with an id
    const DXILFunctionDeclaration* GetFunctionDeclaration(uint32_t id);

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
    bool TryParseIntrinsic(IL::BasicBlock *basicBlock, LLVMRecordReader &reader, uint32_t anchor, uint32_t called, uint32_t result, const DXILFunctionDeclaration *declaration);

private:
    /// All function declarations
    TrivialStackVector<DXILFunctionDeclaration, 32> functions;

    /// All internally linked declaration indices
    TrivialStackVector<uint32_t, 32> internalLinkedFunctions;
};
