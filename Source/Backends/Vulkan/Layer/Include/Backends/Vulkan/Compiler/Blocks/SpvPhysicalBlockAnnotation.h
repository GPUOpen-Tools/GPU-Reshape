#pragma once

// Layer
#include "SpvValueDecoration.h"
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

    /// Check if a value has been decorated
    bool IsDecorated(IL::ID value) const {
        return value < entries.size() && entries[value].decorated;
    }

    /// Check if a value is a binding decoration
    bool IsDecoratedBinding(IL::ID value) const {
        return IsDecorated(value) && entries[value].value.descriptorSet != UINT32_MAX;
    }

    /// Get a decoration
    /// \param value value to be fetched from, must be decorated
    /// \return lifetime tied to block
    const SpvValueDecoration& GetDecoration(IL::ID value) {
        ASSERT(IsDecorated(value), "Invalid decoration");
        return entries[value].value;
    }

    /// Bound number of descriptor sets
    uint32_t boundDescriptorSets{0};

private:
    struct SpvDecorationEntry {
        /// Is this slow decorated?
        bool decorated{false};

        /// Decoration value
        SpvValueDecoration value;
    };

    /// All value entries
    std::vector<SpvDecorationEntry> entries;
};
