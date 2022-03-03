#pragma once

// Layer
#include <Backends/Vulkan/Compiler/SpvTypeMap.h>
#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockSection.h>

// Backend
#include <Backend/IL/Source.h>
#include <Backend/IL/Program.h>

// Std
#include <set>

// Forward declarations
struct SpvPhysicalBlockTable;
struct SpvParseContext;
struct SpvPhysicalBlock;

/// Type constant and variable physical block
struct SpvPhysicalBlockTypeConstantVariable : public SpvPhysicalBlockSection {
    SpvPhysicalBlockTypeConstantVariable(const Allocators &allocators, Backend::IL::Program &program, SpvPhysicalBlockTable& table);

    /// Parse all instructions
    void Parse();

    /// Assign all type associations for a given instruction, ensures types are mapped correctly
    /// \param ctx parsing context
    void AssignTypeAssociation(SpvParseContext& ctx);

    /// Compile the block
    /// \param idMap the shared identifier map for proxies
    void Compile(SpvIdMap& idMap);

    /// Copy to a new block
    /// \param remote the remote table
    /// \param out destination type constant variable
    void CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockTypeConstantVariable& out);

    /// SPIRV type map
    SpvTypeMap typeMap;
};
