#pragma once

// Layer
#include <Backends/Vulkan/Compiler/Blocks/SpvValueDecoration.h>
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

    /// Get the PRMT offset for a given resource
    /// \param stream current stream
    /// \param resource resource to be tracked
    /// \return allocated identifier
    IL::ID GetResourcePRMTOffset(SpvStream& stream, IL::ID resource);

    /// Export a given value
    /// \param stream the current spirv stream
    /// \param value the resource id
    void GetToken(SpvStream& stream, IL::ID resource, IL::ID result);

    /// Copy to a new block
    /// \param remote the new block table
    /// \param out the destination shader export
    void CopyTo(SpvPhysicalBlockTable& remote, SpvUtilShaderPRMT& out);

private:
    /// Find the originating resource decoration
    /// \param resource resource to be traced
    /// \return found decoration
     SpvValueDecoration GetSourceResourceDecoration(IL::ID resource);

private:
    /// Backend program
    Backend::IL::Program &program;

    /// Shared allocators
    Allocators allocators;

    /// Parent table
    SpvPhysicalBlockTable& table;

    /// Spv identifiers
    uint32_t prmTableId{0};

    /// Type map
    const Backend::IL::Type *buffer32UIPtr{nullptr};
    const Backend::IL::Type *buffer32UI{nullptr};
};
