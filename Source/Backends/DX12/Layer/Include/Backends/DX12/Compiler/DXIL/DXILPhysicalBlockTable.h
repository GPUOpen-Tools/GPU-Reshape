#pragma once

// Layer
#include "DXILPhysicalBlockScan.h"
#include "Blocks/DXILPhysicalBlockFunction.h"
#include "Blocks/DXILPhysicalBlockType.h"
#include "Blocks/DXILPhysicalBlockGlobal.h"

struct DXILPhysicalBlockTable {
    DXILPhysicalBlockTable(const Allocators &allocators, Backend::IL::Program &program);

    /// Scan the DXIL bytecode
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
};
