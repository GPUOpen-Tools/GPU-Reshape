#include <Backends/Vulkan/Compiler/SpvSourceMap.h>

void SpvSourceMap::AddPhysicalSource(SpvId id, SpvSourceLanguage language, uint32_t version, const std::string_view& filename) {
    PhysicalSource& source = GetOrAllocate(id);
    source.language = language;
    source.version = version;
    source.filename = filename;
}

void SpvSourceMap::AddSource(SpvId id, const std::string_view &code) {
    PhysicalSource& source = GetOrAllocate(id);

    // Append new fragment
    Fragment& fragment = source.fragments.emplace_back();
    fragment.source = code;

    // First line
    fragment.lineOffsets.push_back(0);

    // Summarize the line offsets
    for (uint32_t i = 0; i < code.length(); i++) {
        if (code[i] == '\n') {
            fragment.lineOffsets.push_back(i);
        }
    }
}

SpvSourceMap::PhysicalSource &SpvSourceMap::GetOrAllocate(SpvId id) {
    if (sourceMappings.size() <= id) {
        sourceMappings.resize(id + 1, InvalidSpvId);
    }

    if (sourceMappings[id] == InvalidSpvId) {
        sourceMappings[id] = static_cast<uint32_t>(physicalSources.size());
        return physicalSources.emplace_back();
    }

    return physicalSources.at(sourceMappings.at(id));
}

SpvId SpvSourceMap::GetPendingSource() const {
    return static_cast<SpvId>(physicalSources.size() - 1);
}

std::string_view SpvSourceMap::GetLine(uint32_t fileIndex, uint32_t line) const {
    const PhysicalSource& source = physicalSources.at(fileIndex);

    // TODO: Maybe this could be optimized

    // Current offset
    uint32_t fragmentLineOffset = 0;

    // Find appropriate fragment
    for (const Fragment& fragment : source.fragments) {
        uint32_t next = fragmentLineOffset + static_cast<uint32_t>(fragment.lineOffsets.size());

        // Within bounds?
        if (line < next) {
            uint32_t offset = fragment.lineOffsets.at(line - fragmentLineOffset);

            // Segment offsets
            const char* begin = &fragment.source[offset] + 1;
            const char* end = std::strchr(begin, '\n');

            // Determine length
            uint32_t length = static_cast<uint32_t>(end ? (end - begin) : (fragment.source.length() - offset));

            return std::string_view(begin, length);
        }

        fragmentLineOffset = next;
    }

    // Not found
    return {};
}
