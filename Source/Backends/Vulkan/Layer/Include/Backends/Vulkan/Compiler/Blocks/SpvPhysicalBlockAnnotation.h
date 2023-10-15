// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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
