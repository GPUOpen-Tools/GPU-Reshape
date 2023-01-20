#pragma once

// Std
#include <cstdint>

struct RootSignatureUserMapping {
    /// Root parameter this is bound to
    uint32_t rootParameter{UINT32_MAX};

    /// Descriptor offset within the parameter
    uint32_t offset{UINT32_MAX};

    /// Unbounded?
    bool isUnbounded{false};
};
