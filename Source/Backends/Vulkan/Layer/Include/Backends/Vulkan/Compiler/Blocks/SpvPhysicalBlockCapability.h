#pragma once

// Layer
#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockSection.h>
#include <Backends/Vulkan/Compiler/Spv.h>

// Backend
#include <Backend/IL/Source.h>
#include <Backend/IL/Program.h>

// Std
#include <set>

// Common
#include <Common/Allocators.h>

// Forward declarations
struct SpvPhysicalBlockScan;
struct SpvPhysicalBlock;
struct SpvPhysicalBlockTable;

/// Capability physical block
struct SpvPhysicalBlockCapability : public SpvPhysicalBlockSection {
    using SpvPhysicalBlockSection::SpvPhysicalBlockSection;

    /// Parse all instructions
    void Parse();

    /// Add a new capability
    /// \param capability capability to be added
    void Add(SpvCapability capability);

    /// Copy to a new block
    /// \param remote the remote table
    /// \param out destination capability
    void CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockCapability& out);

private:
    /// Current set of capabilities
    std::set<SpvCapability> capabilities;
};
