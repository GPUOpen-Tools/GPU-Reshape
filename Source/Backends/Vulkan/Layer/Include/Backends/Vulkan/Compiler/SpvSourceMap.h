#pragma once

#include <string>
#include <vector>

// Spirv
#include "Spv.h"

struct SpvSourceMap {
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

    /// Get the root filename
    /// \return
    std::string_view GetFilename() const;

    /// Get the last pending source spv identifier
    SpvId GetPendingSource() const;

    /// Get the file index for a given spv identifier (physical source)
    uint32_t GetFileIndex(SpvId id) const {
        return sourceMappings.at(id);
    }

private:
    /// Fragment of source code
    struct Fragment {
        std::string_view source;

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
    PhysicalSource& GetOrAllocate(SpvId id);

private:
    /// All sections
    std::vector<PhysicalSource> physicalSources;

    /// All section mappings, to have monotonically incrementing sections
    std::vector<uint32_t> sourceMappings;
};
