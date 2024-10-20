// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
#include "Spv.h"
#include "SpvSourceAssociation.h"

// Common
#include <Common/Allocators.h>
#include <Common/Assert.h>

// Std
#include <string>
#include <vector>
#include <unordered_map>

struct SpvSourceMap {
    /// Constructor
    SpvSourceMap(const Allocators& allocators);
    
    /// Add a new physical source section, may represent a single file
    /// \param id the spv identifier used
    /// \param language the source language
    /// \param version the version of the source language
    /// \param filename the filename of the section
    /// \return the file uid
    uint32_t AddPhysicalSource(SpvId id, SpvSourceLanguage language, uint32_t version, const std::string_view& filename);

    /// Add a source fragment to a physical source
    /// \param id the spv identifier, must have an associated physical source
    /// \param source the source code to be added
    void AddSource(SpvId id, const std::string_view& source);

    /// Finalize the source map
    void Finalize();

    /// Get a specific line from this source map
    /// \param fileIndex the file index
    /// \param line the line to be fetched
    /// \return the line, empty if OOB
    std::string_view GetLine(uint32_t fileIndex, uint32_t line) const;

    /// Add a source association mapping
    /// \param id spirv identifier
    /// \param mapping the mapping
    void AddSourceAssociation(uint32_t sourceOffset, const SpvSourceAssociation& association);

    /// Get the source association for an id
    /// \param id spv identifier
    /// \return empty if not found
    SpvSourceAssociation GetSourceAssociation(uint32_t sourceOffset) const;

    /// Get the root filename
    /// \return
    std::string_view GetFilename() const;

    /// Get the source filename
    /// \return filename
    std::string_view GetSourceFilename(uint32_t fileUID) const;

    /// Get the total size of the combined source code
    /// \return length, not null terminated
    uint64_t GetCombinedSourceLength(uint32_t fileUID) const;

    /// Fill the combined source code into an output buffer
    /// \param buffer length must be at least GetCombinedSourceLength
    void FillCombinedSource(uint32_t fileUID, char* buffer) const;

    /// Get the number of files
    uint32_t GetFileCount() const {
        return static_cast<uint32_t>(physicalSources.size());
    }

    /// Get the file index for a given spv identifier (physical source)
    uint32_t GetFileIndex(SpvId id, const std::string_view& view) {
        if (id >= sourceMappings.size()) {
            sourceMappings.resize(id + 1, InvalidSpvId);
        }

        // Get mapping
        uint32_t& index = sourceMappings.at(id);

        // Not assigned?
        if (index == InvalidSpvId) {
            index = Find(view);
            ASSERT(index != InvalidSpvId, "Failed to associate shader source");
        }

        // OK
        return index;
    }
    
private:
    /// Fragment of source code
    struct Fragment {
        /// Source view of this fragment
        std::string_view source;

        /// Starting line
        uint32_t lineStart{0};

        /// Starting offset within the parent fragment
        uint32_t fragmentInlineOffset{0};

        /// All line offsets
        std::vector<uint32_t> lineOffsets;
    };

    /// Complete source section
    struct PhysicalSource {
        /// Source language
        SpvSourceLanguage language{SpvSourceLanguageMax};

        /// Source version
        uint32_t version{0};

        /// Filename of this source
        std::string filename;

        /// Code wise view
        std::string_view source;
        
        /// All pending chunks for finalization
        std::vector<std::string_view> pendingSourceChunks;

        /// Optional storage for multi-chunk sources
        std::string sourceStorage;

        /// All final fragments
        std::vector<Fragment> fragments;
    };

    /// Get a source section
    uint32_t Find(const std::string_view& view);

    /// Get a source section
    PhysicalSource* GetOrAllocate(const std::string_view& view, SpvId id);

    /// Get a source section
    PhysicalSource* GetOrAllocate(const std::string_view& view);

    /// Finalize a given source
    void FinalizeSource(PhysicalSource* source);

    /// Finalize all fragments within a source
    void FinalizeFragments(PhysicalSource* source);

private:
    Allocators allocators;
    
    /// All sections
    std::vector<PhysicalSource*> physicalSources;

    /// All mappings
    std::unordered_map<uint32_t, SpvSourceAssociation> sourceAssociations;

    /// All section mappings, to have monotonically incrementing sections
    std::vector<uint32_t> sourceMappings;
};
