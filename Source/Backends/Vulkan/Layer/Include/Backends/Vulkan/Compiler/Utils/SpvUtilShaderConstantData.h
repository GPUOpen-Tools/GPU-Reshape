#pragma once

// Layer
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockScan.h>

// Backend
#include <Backend/IL/Source.h>
#include <Backend/IL/Type.h>
#include <Backend/IL/Program.h>

// Forward declarations
struct SpvJob;
struct SpvPhysicalBlockTable;

/// Shader constant utilities
struct SpvUtilShaderConstantData {
    SpvUtilShaderConstantData(const Allocators &allocators, Backend::IL::Program &program, SpvPhysicalBlockTable& table);

    /// Compile the records
    /// \param job source job being compiled against
    void CompileRecords(const SpvJob &job);

    /// Export a given value
    /// \param stream the current spirv stream
    /// \param value the resource id
    IL::ID GetConstantData(SpvStream& stream, uint32_t index);

    /// Copy to a new block
    /// \param remote the new block table
    /// \param out the destination shader block
    void CopyTo(SpvPhysicalBlockTable& remote, SpvUtilShaderConstantData& out);

private:
    /// Shared allocators
    Allocators allocators;

    /// Backend program
    Backend::IL::Program &program;

    /// Parent table
    SpvPhysicalBlockTable& table;

    /// Spv identifiers
    uint32_t constantId{0};
};
