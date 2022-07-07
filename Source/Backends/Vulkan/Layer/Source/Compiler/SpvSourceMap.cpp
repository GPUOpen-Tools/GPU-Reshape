#include <Backends/Vulkan/Compiler/SpvSourceMap.h>

void SpvSourceMap::SetBound(SpvId bound) {
    /* */
}

void SpvSourceMap::AddPhysicalSource(SpvId id, SpvSourceLanguage language, uint32_t version, const std::string_view &filename) {
    PhysicalSource &source = GetOrAllocate(filename, id);
    source.language = language;
    source.version = version;
    source.filename = filename;
}

void SpvSourceMap::AddSource(SpvId id, const std::string_view &code) {
    PhysicalSource &source = physicalSources.at(sourceMappings.at(id));

    // Original source info
    SpvSourceLanguage language = source.language;
    uint32_t version = source.version;

    // Append initial fragment
    Fragment *fragment = &source.fragments.emplace_back();
    fragment->source = code;
    fragment->lineOffsets.push_back(0);
    fragment->lineStart = 0;

    // Summarize the line offsets
    for (uint32_t i = 0; i < code.length(); i++) {
        if (code[i] == '#') {
            constexpr const char *kLineDirective = "#line";

            // Is line directive?
            if (std::strncmp(&code[i], kLineDirective, std::strlen(kLineDirective))) {
                continue;
            }

            // Eat until number
            while (!isdigit(code[i]) && code[i]) {
                i++;
            }

            // Parse line offset
            const uint32_t offset = atoi(&code[i]);

            // Eat until start of string
            while (code[i] != '"' && code[i]) {
                i++;
            }

            // Eat "
            i++;

            // Eat until end of string
            const uint32_t start = i;
            while (code[i] != '"' && code[i]) {
                i++;
            }

            // Get filename
            std::string_view file = code.substr(start, i - start);

            // Eat until next line
            while (code[i] != '\n' && code[i]) {
                i++;
            }

            // Next line
            i++;

            PhysicalSource &extendedSource = GetOrAllocate(file);
            source.filename = file;
            source.version = version;
            source.language = language;

            fragment = &extendedSource.fragments.emplace_back();
            fragment->source = code;
            fragment->lineOffsets.push_back(i);
            fragment->lineStart = offset - 1;
        }

        if (code[i] == '\n') {
            fragment->lineOffsets.push_back(i + 1);
        }
    }
}

SpvSourceMap::PhysicalSource &SpvSourceMap::GetOrAllocate(const std::string_view &view, SpvId id) {
    if (sourceMappings.size() <= id) {
        sourceMappings.resize(id + 1, InvalidSpvId);
    }

    for (PhysicalSource &source: physicalSources) {
        if (source.filename == view) {
            return source;
        }
    }

    for (uint32_t i = 0; i < physicalSources.size(); i++) {
        PhysicalSource &source = physicalSources[i];

        if (source.filename == view) {
            sourceMappings[id] = i;
            return source;
        }
    }

    sourceMappings[id] = static_cast<uint32_t>(physicalSources.size());
    return physicalSources.emplace_back();
}

SpvSourceMap::PhysicalSource &SpvSourceMap::GetOrAllocate(const std::string_view &view) {
    for (PhysicalSource &source: physicalSources) {
        if (source.filename == view) {
            return source;
        }
    }

    return physicalSources.emplace_back();
}

std::string_view SpvSourceMap::GetLine(uint32_t fileIndex, uint32_t line) const {
    const PhysicalSource &source = physicalSources.at(fileIndex);

    // TODO: Maybe this could be optimized

    // Find appropriate fragment
    for (const Fragment &fragment: source.fragments) {
        // Within bounds?
        if (line >= fragment.lineStart && line < fragment.lineStart + fragment.lineOffsets.size()) {
            uint32_t offset = fragment.lineOffsets.at(line - fragment.lineStart);

            // Segment offsets
            const char *begin = &fragment.source[offset];
            const char *end = std::strchr(begin, '\n');

            // Determine length
            uint32_t length = static_cast<uint32_t>(end ? (end - begin) : (fragment.source.length() - offset));

            return std::string_view(begin, length);
        }
    }

    // Not found
    return {};
}

std::string_view SpvSourceMap::GetFilename() const {
    if (physicalSources.empty()) {
        return {};
    }

    return physicalSources[0].filename;
}

void SpvSourceMap::AddSourceAssociation(uint32_t sourceOffset, const SpvSourceAssociation &association) {
    sourceAssociations[sourceOffset] = association;
}

SpvSourceAssociation SpvSourceMap::GetSourceAssociation(uint32_t sourceOffset) const {
    auto it = sourceAssociations.find(sourceOffset);
    if (it == sourceAssociations.end()) {
        return {};
    }

    return it->second;
}

uint64_t SpvSourceMap::GetCombinedSourceLength(uint32_t fileUID) const {
    const PhysicalSource &source = physicalSources[fileUID];

    // Not null terminated
    uint64_t length = 0;

    // Sum length
    for (const Fragment &fragment: source.fragments) {
        length += fragment.source.length();
    }

    return length;
}

void SpvSourceMap::FillCombinedSource(uint32_t fileUID, char *buffer) const {
    const PhysicalSource &source = physicalSources[fileUID];

    // Copy memory
    for (const Fragment &fragment: source.fragments) {
        std::memcpy(buffer, fragment.source.data(), fragment.source.length() * sizeof(char));
        buffer += fragment.source.length();
    }
}
