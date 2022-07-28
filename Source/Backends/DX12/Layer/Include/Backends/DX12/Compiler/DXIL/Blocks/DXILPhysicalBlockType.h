#pragma once

// Layer
#include "DXILPhysicalBlockSection.h"
#include "Backends/DX12/Compiler/DXIL/DXILTypeMap.h"

/// Type block
struct DXILPhysicalBlockType : public DXILPhysicalBlockSection {
    DXILPhysicalBlockType(const Allocators &allocators, Backend::IL::Program &program, DXILPhysicalBlockTable &table);

    void CopyTo(DXILPhysicalBlockType& out);

public:
    /// Parse all instructions
    void ParseType(const struct LLVMBlock *block);

public:
    /// Compile all instructions
    void CompileType(LLVMBlock *block);

public:
    /// Stitch all instructions
    void StitchType(LLVMBlock *block);

    /// Type mapper
    DXILTypeMap typeMap;

private:
    // Incremented type
    uint32_t typeAlloc{0};
};
