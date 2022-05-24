#pragma once

// Layer
#include "DXILPhysicalBlockSection.h"

/// Function block
struct DXILPhysicalBlockFunction : public DXILPhysicalBlockSection {
    using DXILPhysicalBlockSection::DXILPhysicalBlockSection;

    /// Parse all instructions
    void Parse();

private:
    /// Parse a block
    void ParseBlock(IL::Function* fn, const struct LLVMBlock* block);

    /// Parse all constants
    void ParseConstants(const struct LLVMBlock* block);
};
