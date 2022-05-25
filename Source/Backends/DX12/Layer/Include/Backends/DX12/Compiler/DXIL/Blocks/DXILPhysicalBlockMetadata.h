#pragma once

// Layer
#include "DXILPhysicalBlockSection.h"
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBlock.h>

// Std
#include <string_view>
#include <vector>

/// Type block
struct DXILPhysicalBlockMetadata : public DXILPhysicalBlockSection {
    using DXILPhysicalBlockSection::DXILPhysicalBlockSection;

    /// Parse all instructions
    void ParseMetadata(const struct LLVMBlock *block);

private:
    struct Metadata {
        /// Name associated
        std::string name;
    };

    /// All hosted metadata
    std::vector<Metadata> metadata;
};
