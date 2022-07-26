#pragma once

// Layer
#include "DXILPhysicalBlockSection.h"

// Std
#include <string_view>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBlock.h>

/// Type block
struct DXILPhysicalBlockString : public DXILPhysicalBlockSection {
    using DXILPhysicalBlockSection::DXILPhysicalBlockSection;

public:
    /// Parse all instructions
    void ParseStrTab(const struct LLVMBlock *block);

    /// Get string
    /// \param offset source offset
    /// \param length length of segment
    /// \return view
    std::string_view GetString(uint64_t offset, uint64_t length);

public:
    /// Compile all instructions
    void CompileStrTab(struct LLVMBlock *block);

    /// Stitch all instructions
    void StitchStrTab(struct LLVMBlock *block);

private:
    const struct LLVMRecord* blobRecord{nullptr};
};
