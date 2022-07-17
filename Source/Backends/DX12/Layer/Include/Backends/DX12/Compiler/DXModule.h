#pragma once

// Layer
#include <Backends/DX12/DX12.h>

// Common
#include <Common/GlobalUID.h>

// Std
#include <cstdint>

// Forward declarations
namespace IL {
    struct Program;
}

/// Base DXModule
class DXModule {
public:
    virtual ~DXModule() = default;

    /// Scan the DXIL bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    virtual bool Parse(const void* byteCode, uint64_t byteLength) = 0;

    /// Copy this module
    /// \return
    virtual DXModule* Copy() = 0;

    /// Get the program of this module
    /// \return program
    virtual IL::Program* GetProgram() = 0;

    /// Get the guid
    /// \return
    virtual GlobalUID GetInstrumentationGUID() = 0;
};
