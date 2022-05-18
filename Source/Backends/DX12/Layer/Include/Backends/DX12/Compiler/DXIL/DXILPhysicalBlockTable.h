#pragma once

// Layer
#include "DXILPhysicalBlockScan.h"
#include "Blocks/DXILPhysicalBlockFunction.h"

struct DXILPhysicalBlockTable {
    DXILPhysicalBlockTable() : function(*this) {
        /* */
    }

    /// Scan the DXIL bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    bool Parse(const void* byteCode, uint64_t byteLength);

    /// Scanner
    DXILPhysicalBlockScan scan;

    /// Blocks
    DXILPhysicalBlockFunction function;
};
