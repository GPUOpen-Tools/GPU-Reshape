#pragma once

// Layer
#include "DXBCPhysicalBlockScan.h"
#include "Blocks/DXBCPhysicalBlockShader.h"

// Forward declarations
struct DXILModule;

/// Complete physical block table
struct DXBCPhysicalBlockTable {
    DXBCPhysicalBlockTable() : shader(*this) {
        /* */
    }

    /// Scan the DXBC bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    bool Parse(const void* byteCode, uint64_t byteLength);

    /// Scanner
    DXBCPhysicalBlockScan scan;

    /// Blocks
    DXBCPhysicalBlockShader shader;

    /// DXBC containers can host DXIL data
    DXILModule* dxilModule{nullptr};
};
