#pragma once

// Layer
#include "SpvPhysicalBlockSection.h"

// Program
#include <Backend/IL/Program.h>

// Common
#include <Common/Allocators.h>

// Std
#include <set>

// Forward declarations
struct SpvPhysicalBlockScan;
struct SpvPhysicalBlock;
struct SpvPhysicalBlockTable;

/// Annotation physical block
struct SpvPhysicalBlockAnnotation : public SpvPhysicalBlockSection {
    using SpvPhysicalBlockSection::SpvPhysicalBlockSection;

    /// Parse the annotation block
    void Parse();

    /// Copy to an annotation block
    /// \param remote the remote table
    /// \param out destination annotation
    void CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockAnnotation& out);

    /// Bound number of descriptor sets
    uint32_t boundDescriptorSets{0};
};
