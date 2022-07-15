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

    /// Get constant at offset
    const IL::Constant* GetConstant(uint32_t id) {
        if (id >= constants.size()) {
            return nullptr;
        }

        return constants[id];
    }

private:
    /// All constants
    std::vector<const IL::Constant*> constants;
};
