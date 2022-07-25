#pragma once

// Layer
#include "DXILPhysicalBlockSection.h"
#include <Backends/DX12/Compiler/DXIL/DXILConstantMap.h>

/// Type block
struct DXILPhysicalBlockGlobal : public DXILPhysicalBlockSection {
    DXILPhysicalBlockGlobal(const Allocators &allocators, Backend::IL::Program &program, DXILPhysicalBlockTable &table);

    /// Copy this block
    /// \param out destination block
    void CopyTo(DXILPhysicalBlockGlobal& out);

public:
    /// Parse a constant block
    /// \param block source block
    void ParseConstants(const struct LLVMBlock* block);

    /// Parse a global variable
    /// \param record source record
    void ParseGlobalVar(const struct LLVMRecord& record);

    /// Parse an alias
    /// \param record source record
    void ParseAlias(const struct LLVMRecord& record);

public:
    /// Compile a constant block
    /// \param block block
    void CompileConstants(struct LLVMBlock* block);

    /// Compile a global variable
    /// \param record record
    void CompileGlobalVar(struct LLVMRecord& record);

    /// Compile an alias
    /// \param record record
    void CompileAlias(struct LLVMRecord& record);

private:
    /// Underlying constants
    DXILConstantMap constantMap;
};
