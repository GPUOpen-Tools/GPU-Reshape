#pragma once

// Layer
#include "DXILPhysicalBlockSection.h"
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBlock.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordStringView.h>

/// Type block
struct DXILPhysicalBlockSymbol : public DXILPhysicalBlockSection {
    using DXILPhysicalBlockSection::DXILPhysicalBlockSection;

    /// Parse all records
    void ParseSymTab(const struct LLVMBlock *block);

    /// Get a value mapping
    /// \param id value id
    /// \return empty if not found
    LLVMRecordStringView GetValueString(uint32_t id);

private:
    std::vector<LLVMRecordStringView> valueStrings;
};
