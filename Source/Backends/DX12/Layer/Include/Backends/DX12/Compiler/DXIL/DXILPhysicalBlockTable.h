#pragma once

// Layer
#include "DXILPhysicalBlockScan.h"
#include "Blocks/DXILPhysicalBlockFunction.h"
#include "Blocks/DXILPhysicalBlockType.h"
#include "Blocks/DXILPhysicalBlockGlobal.h"
#include "Blocks/DXILPhysicalBlockString.h"
#include "Blocks/DXILPhysicalBlockMetadata.h"
#include "Blocks/DXILPhysicalBlockSymbol.h"
#include "DXILIDMap.h"

// Std
#include <string>

struct DXILPhysicalBlockTable {
    DXILPhysicalBlockTable(const Allocators &allocators, Backend::IL::Program &program);

    /// Parse the DXIL bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    bool Parse(const void* byteCode, uint64_t byteLength);

    /// Scanner
    DXILPhysicalBlockScan scan;

    /// Blocks
    DXILPhysicalBlockFunction function;
    DXILPhysicalBlockType type;
    DXILPhysicalBlockGlobal global;
    DXILPhysicalBlockString string;
    DXILPhysicalBlockSymbol symbol;
    DXILPhysicalBlockMetadata metadata;

    /// Shared identifier map
    DXILIDMap idMap;

    /// LLVM triple
    std::string triple;

    /// LLVM data layout
    std::string dataLayout;
};
