#pragma once

// Layer
#include "DXILPhysicalBlockSection.h"
#include "Backends/DX12/Compiler/DXIL/DXILTypeMap.h"

/// Type block
struct DXILPhysicalBlockType : public DXILPhysicalBlockSection {
    DXILPhysicalBlockType(const Allocators &allocators, Backend::IL::Program &program, DXILPhysicalBlockTable &table);

    /// Parse all instructions
    void Parse();

private:
    DXILTypeMap typeMap;
};
