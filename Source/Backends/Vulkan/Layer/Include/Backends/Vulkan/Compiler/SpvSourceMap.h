#pragma once

// Std
#include <string>
#include <vector>
#include <map>

// Layer
#include "Spv.h"
#include "SpvSourceAssociation.h"

struct SpvSourceMap {
    /// Set the spv bound
    /// \param bound
    void SetBound(SpvId bound);

    /// Add a new physical source section, may represent a single file
    /// \param id the spv identifier used
    /// \param language the source language
    /// \param version the version of the source language
    /// \param filename the filename of the section
    void AddPhysicalSource(SpvId id, SpvSourceLanguage language, uint32_t version, const std::string_view& filename);

    /// Add a source fragment to a physical source
    /// \param id the spv identifier, must have an associated physical source
    /// \param source the source code to be added
    void AddSource(SpvId id, const std::string_view& source);

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

    /// Get the file index for a given spv identifier (physical source)
    uint32_t GetFileIndex(SpvId id) const {
        return sourceMappings.at(id);
    }

private:
    /// Fragment of source code
    struct Fragment {
        std::string_view source;

        uint32_t lineStart{0};
        std::vector<uint32_t> lineOffsets;
    };

    /// Complete source section
    struct PhysicalSource {
        SpvSourceLanguage language{SpvSourceLanguageMax};
        uint32_t version{0};
        std::string_view filename;
        std::vector<Fragment> fragments;
    };

    /// Get a source section
    PhysicalSource& GetOrAllocate(const std::string_view& view, SpvId id);

    /// Get a source section
    PhysicalSource& GetOrAllocate(const std::string_view& view);

private:
    /// All sections
    std::vector<PhysicalSource> physicalSources;

    /// All mappings
    std::map<uint32_t, SpvSourceAssociation> sourceAssociations;

    /// All section mappings, to have monotonically incrementing sections
    std::vector<uint32_t> sourceMappings;
};
