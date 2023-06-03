#pragma once

// Common
#include <Common/ComRef.h>

// Forward declarations
class PDBController;

/// Job description
struct DXParseJob {
    /// Source byte code
    const void* byteCode{nullptr};

    /// Source byte count
    uint64_t byteLength{UINT64_MAX};

    /// Controllers
    ComRef<PDBController> pdbController;
};
