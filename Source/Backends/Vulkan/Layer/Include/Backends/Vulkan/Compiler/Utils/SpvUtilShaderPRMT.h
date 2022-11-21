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

/// Shader PRMT utilities
struct SpvUtilShaderPRMT {
    SpvUtilShaderPRMT(const Allocators &allocators, Backend::IL::Program &program, SpvPhysicalBlockTable& table);

    /// Compile the records
    /// \param job source job being compiled against
    void CompileRecords(const SpvJob &job);

    /// Export a given value
    /// \param stream the current spirv stream
    /// \param exportID the <compile time> identifier
    /// \param value the value to be exported
    void Export(SpvStream& stream, uint32_t exportID, IL::ID value);

    /// Copy to a new block
    /// \param remote the new block table
    /// \param out the destination shader export
    void CopyTo(SpvPhysicalBlockTable& remote, SpvUtilShaderPRMT& out);

private:
    /// Backend program
    Backend::IL::Program &program;

    /// Shared allocators
    Allocators allocators;

    /// Parent table
    SpvPhysicalBlockTable& table;

    /// Spv identifiers
    uint32_t counterId{0};
    uint32_t streamId{0};

    /// Type map
    const Backend::IL::Type *buffer32UIRWArrayPtr{nullptr};
    const Backend::IL::Type *buffer32UIRWPtr{nullptr};
    const Backend::IL::Type *buffer32UIRW{nullptr};
};
