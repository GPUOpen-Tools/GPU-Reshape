#pragma once

// Layer
#include "SpvInstruction.h"
#include "SpvPhysicalBlock.h"
#include "SpvHeader.h"

// Generated
#include <Backends/Vulkan/Compiler/Spv.Gen.h>

// Backend
#include <Backend/IL/Program.h>

// Std
#include <algorithm>

/// Physical block scanner
struct SpvPhysicalBlockScan {
    SpvPhysicalBlockScan(IL::Program& program);

    /// Scan a spirv stream
    /// \param code stream begin
    /// \param count stream word count
    /// \return success state
    bool Scan(const uint32_t *const code, uint32_t count);

    /// Get the expected module word count
    /// \return word count
    uint64_t GetModuleWordCount() const;

    /// Stitch a program
    /// \param out the destination stream
    void Stitch(SpvStream& out);

    /// Copy this block scanner
    /// \param out destination scanner
    void CopyTo(SpvPhysicalBlockScan& out);

    /// Get a physical block
    /// \param type given type of the block
    /// \return the physical block
    SpvPhysicalBlock* GetPhysicalBlock(SpvPhysicalBlockType type) {
        Section& section = sections[static_cast<uint32_t>(type)];
        return &section.physicalBlock;
    }

    /// SPIRV header
    SpvHeader header;

private:
    struct Section {
        SpvPhysicalBlock physicalBlock;
    };

    /// Backend program
    IL::Program& program;

    /// All sections
    Section sections[static_cast<uint32_t>(SpvPhysicalBlockType::Count)];
};
