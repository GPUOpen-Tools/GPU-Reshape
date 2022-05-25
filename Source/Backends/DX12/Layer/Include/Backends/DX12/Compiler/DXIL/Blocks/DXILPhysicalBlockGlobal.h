#pragma once

// Layer
#include "DXILPhysicalBlockSection.h"

/// Type block
struct DXILPhysicalBlockGlobal : public DXILPhysicalBlockSection {
    using DXILPhysicalBlockSection::DXILPhysicalBlockSection;

    /// Parse a constant block
    /// \param block source block
    void ParseConstants(const struct LLVMBlock* block);

    /// Parse a global variable
    /// \param record source record
    void ParseGlobalVar(const struct LLVMRecord& record);

    /// Parse an alias
    /// \param record source record
    void ParseAlias(const struct LLVMRecord& record);
};
