#pragma once

// System
#include <d3d12.h>

// Std
#include <cstdint>

/// Base DXModule
class DXModule {
public:
    /// Scan the DXIL bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    virtual bool Parse(const void* byteCode, uint64_t byteLength) = 0;
};
