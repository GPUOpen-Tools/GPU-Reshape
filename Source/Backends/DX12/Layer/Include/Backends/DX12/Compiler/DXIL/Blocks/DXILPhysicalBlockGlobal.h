#pragma once

// Layer
#include "DXILPhysicalBlockSection.h"

/// Type block
struct DXILPhysicalBlockGlobal : public DXILPhysicalBlockSection {
    using DXILPhysicalBlockSection::DXILPhysicalBlockSection;

    /// Parse all instructions
    void Parse();
};
