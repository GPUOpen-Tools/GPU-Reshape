#pragma once

// Layer
#include "DXILPhysicalBlockSection.h"
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBlock.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordStringView.h>

// Common
#include <Common/Containers/LinearBlockAllocator.h>

/// Type block
struct DXILPhysicalBlockSymbol : public DXILPhysicalBlockSection {
    DXILPhysicalBlockSymbol(const Allocators &allocators, Backend::IL::Program &program, DXILPhysicalBlockTable &table);

    /// Copy this block
    /// \param out destination block
    void CopyTo(DXILPhysicalBlockSymbol& out);

public:
    /// Parse all records
    void ParseSymTab(const struct LLVMBlock *block);

    /// Get a value mapping
    /// \param id value id
    /// \return empty if not found
    LLVMRecordStringView GetValueString(uint32_t id);

    /// Get a value cstring mapping, may incur an allocation
    /// \param id value id
    /// \return nullptr if not found
    const char* GetValueAllocation(uint32_t id);

public:
    /// Compile all records
    void CompileSymTab(struct LLVMBlock *block);

    /// Stitch all records
    void StitchSymTab(struct LLVMBlock *block);

private:
    /// Raw values
    std::vector<LLVMRecordStringView> valueStrings;

    /// String values, allocated on demand
    std::vector<char*> valueAllocations;

    /// Generic allocator
    LinearBlockAllocator<16384> blockAllocator;
};
