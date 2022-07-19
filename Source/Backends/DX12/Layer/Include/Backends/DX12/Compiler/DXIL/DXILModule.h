#pragma once

// Layer
#include <Backends/DX12/Compiler/DXModule.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>

// Backend
#include <Backend/IL/Program.h>

/// DXIL bytecode module
class DXILModule final : public DXModule {
public:
    ~DXILModule();

    /// Construct
    /// \param allocators shared allocators
    DXILModule(const Allocators &allocators);

    /// Construct from top level program
    /// \param allocators shared allocators
    /// \param program the top level program
    DXILModule(const Allocators &allocators, IL::Program* program);

    /// Overrides
    DXModule* Copy() override;
    bool Parse(const void* byteCode, uint64_t byteLength) override;
    IL::Program *GetProgram() override;
    GlobalUID GetInstrumentationGUID() override;
    bool Compile(const DXJob& job, DXStream& out) override;

private:
    /// All physical blocks
    DXILPhysicalBlockTable table;

    /// Program
    IL::Program* program{nullptr};

    /// Nested module?
    bool nested{true};

private:
    Allocators allocators;
};
