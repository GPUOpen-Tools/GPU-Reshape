#pragma once

// Layer
#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockSection.h>
#include <Backends/Vulkan/Compiler/SpvDebugMap.h>
#include <Backends/Vulkan/Compiler/SpvSourceMap.h>

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

/// Debug string source basic block
struct SpvPhysicalBlockDebugStringSource : public SpvPhysicalBlockSection {
    SpvPhysicalBlockDebugStringSource(const Allocators &allocators, Backend::IL::Program &program, SpvPhysicalBlockTable &table);

    /// Parse all instructions
    void Parse();

    /// Copy to a new block
    /// \param remote the remote table
    /// \param out destination debug string source
    void CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockDebugStringSource& out);

    /// SPIRV debug map
    SpvDebugMap debugMap;

    /// SPIRV source map
    SpvSourceMap sourceMap;
};
