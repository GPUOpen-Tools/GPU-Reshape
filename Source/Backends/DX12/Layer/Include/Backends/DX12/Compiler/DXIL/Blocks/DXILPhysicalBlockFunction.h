#pragma once

// Layer
#include <Backends/DX12/Compiler/DXIL/DXILFunctionDeclaration.h>
#include "DXILPhysicalBlockSection.h"


// Common
#include <Common/Containers/TrivialStackVector.h>

/// Function block
struct DXILPhysicalBlockFunction : public DXILPhysicalBlockSection {
    using DXILPhysicalBlockSection::DXILPhysicalBlockSection;

    /// Parse a function
    /// \param block source block
    void ParseFunction(const struct LLVMBlock *block);

    /// Parse a module function
    /// \param record source record
    void ParseModuleFunction(const struct LLVMRecord& record);

    /// Get the declaration associated with an id
    const DXILFunctionDeclaration* GetFunctionDeclaration(uint32_t id);

private:
    /// Does the record have a result?
    bool HasResult(const struct LLVMRecord& record);

private:
    /// All function declarations
    TrivialStackVector<DXILFunctionDeclaration, 32> functions;

    /// All internally linked declaration indices
    TrivialStackVector<uint32_t, 32> internalLinkedFunctions;
};
