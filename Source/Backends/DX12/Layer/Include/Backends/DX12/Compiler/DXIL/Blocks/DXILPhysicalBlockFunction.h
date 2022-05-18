#pragma once

// Layer
#include "DXILPhysicalBlockSection.h"

/// Function block
struct DXILPhysicalBlockFunction : public DXILPhysicalBlockSection {
    using DXILPhysicalBlockSection::DXILPhysicalBlockSection;

    /// Parse all instructions
    void Parse();
};
