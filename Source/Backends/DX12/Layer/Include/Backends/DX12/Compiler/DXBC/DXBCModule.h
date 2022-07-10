#pragma once

// Layer
#include <Backends/DX12/Compiler/DXModule.h>
#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockTable.h>

// Backend
#include <Backend/IL/Program.h>

/// DXBC bytecode module
class DXBCModule final : public DXModule {
public:
    ~DXBCModule();

    /// Construct new program
    /// \param allocators shared allocators
    DXBCModule(const Allocators &allocators);

    /// Construct from shared program
    /// \param allocators shared allocators
    /// \param program top level program
    DXBCModule(const Allocators &allocators, IL::Program* program);

    /// Overrides
    DXModule* Copy() override;
    bool Parse(const void* byteCode, uint64_t byteLength) override;
    IL::Program *GetProgram() override;

private:
    /// Physical table
    DXBCPhysicalBlockTable table;

    /// Program
    IL::Program* program{nullptr};

    /// Nested module?
    bool nested{true};

private:
    Allocators allocators;
};
