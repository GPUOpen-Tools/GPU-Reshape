#pragma once

// Layer
#include "DXILPhysicalBlockSection.h"

/// Type block
struct DXILPhysicalBlockGlobal : public DXILPhysicalBlockSection {
    using DXILPhysicalBlockSection::DXILPhysicalBlockSection;

public:
    /// Parse a constant block
    /// \param block source block
    void ParseConstants(const struct LLVMBlock* block);

    /// Parse a global variable
    /// \param record source record
    void ParseGlobalVar(const struct LLVMRecord& record);

    /// Parse an alias
    /// \param record source record
    void ParseAlias(const struct LLVMRecord& record);

public:
    /// Compile a constant block
    /// \param block block
    void CompileConstants(struct LLVMBlock* block);

    /// Compile a global variable
    /// \param record record
    void CompileGlobalVar(struct LLVMRecord& record);

    /// Compile an alias
    /// \param record record
    void CompileAlias(struct LLVMRecord& record);

public:
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
