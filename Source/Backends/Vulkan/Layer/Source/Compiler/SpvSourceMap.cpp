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

#include <Backends/Vulkan/Compiler/SpvSourceMap.h>

// Common
#include <Common/FileSystem.h>

SpvSourceMap::SpvSourceMap(const Allocators &allocators) : allocators(allocators) {
    
}

static std::string SanitizeCompilerPath(const std::string_view& view) {
    std::string path = SanitizePath(view);

    // Remove dangling delims
    if (path.ends_with("\\")) {
        path.erase(path.end());
    }

    // OK
    return path;
}

void SpvSourceMap::AddPhysicalSource(SpvId id, SpvSourceLanguage language, uint32_t version, const std::string_view &filename) {
    PhysicalSource *source = GetOrAllocate(filename, id);
    source->language = language;
    source->version = version;
    source->filename = SanitizeCompilerPath(filename);
}

void SpvSourceMap::AddSource(SpvId id, const std::string_view &code) {
    PhysicalSource *source = physicalSources.at(sourceMappings.at(id));
    source->pendingSourceChunks.push_back(code);
}

void SpvSourceMap::Finalize() {
    size_t sourceCount = physicalSources.size();
    
    // Finalize all sources
    for (size_t i = 0; i < sourceCount; i++) {
        PhysicalSource* source = physicalSources[i];

        FinalizeSource(source);
        FinalizeFragments(source);
    }
}

void SpvSourceMap::FinalizeSource(PhysicalSource* source) {
    if (source->pendingSourceChunks.size() == 1) {
        source->source = source->pendingSourceChunks[0];
        return;
    }

    // Cursors
    size_t length = 0;
    size_t offset = 0;

    // Precompute length
    for (const std::string_view& pending : source->pendingSourceChunks) {
        length += pending.length();
    } 

    // Preallocate
    source->sourceStorage.resize(length);

    // Copy contents
    for (const std::string_view& pending : source->pendingSourceChunks) {
        std::memcpy(&source->sourceStorage[offset], pending.data(), pending.length());
        offset += pending.length();
    }

    // Set view
    source->source = source->sourceStorage;
}

void SpvSourceMap::FinalizeFragments(PhysicalSource* source) {
    std::string_view code = source->source;
    
    // Original source info
    SpvSourceLanguage language = source->language;
    uint32_t version = source->version;

    // Append initial fragment
    Fragment *fragment = &source->fragments.emplace_back();
    fragment->source = code;
    fragment->lineOffsets.push_back(0);
    fragment->lineStart = 0;
    fragment->fragmentInlineOffset = 0;

    // Summarize the line offsets
    for (uint32_t i = 0; i < code.length(); i++) {
        if (code[i] == '#') {
            constexpr const char *kLineDirective = "#line";

            // Start of directive
            uint32_t directiveStart = i;

            // Is line directive?
            if (std::strncmp(&code[i], kLineDirective, std::strlen(kLineDirective))) {
                continue;
            }

            // Consume last offset
            ASSERT(!fragment->lineOffsets.empty(), "No offset to consume");
            fragment->lineOffsets.pop_back();

            // Eat until number
            while (i < code.length() && !isdigit(code[i])) {
                i++;
            }

            // Parse line offset
            const uint32_t offset = atoi(&code[i]);

            // Eat until start of string
            while (i < code.length() && code[i] != '"') {
                i++;
            }

            // Eat "
            i++;

            // Eat until end of string
            const uint32_t start = i;
            while (i < code.length() && code[i] != '"') {
                i++;
            }

            // Get filename
            std::string file = SanitizeCompilerPath(code.substr(start, i - start));

            // Eat until next line
            while (i < code.length() && code[i] != '\n') {
                i++;
            }

            // Close off previous fragment
            fragment->source = code.substr(fragment->fragmentInlineOffset, directiveStart - fragment->fragmentInlineOffset);

            // Get file
            PhysicalSource *extendedSource = GetOrAllocate(file);
            extendedSource->filename = std::move(file);
            extendedSource->version = version;
            extendedSource->language = language;

            // Extend fragments
            fragment = &extendedSource->fragments.emplace_back();
            fragment->fragmentInlineOffset = i + 1;
            fragment->source = code.substr(fragment->fragmentInlineOffset);
            fragment->lineOffsets.push_back(0);
            fragment->lineStart = offset - 1;
        }

        else if (code[i] == '\n') {
            fragment->lineOffsets.push_back((i - fragment->fragmentInlineOffset) + 1);
        }
    }
}

uint32_t SpvSourceMap::Find(const std::string_view &view) {
    // Sanitize the input path
    std::string sanitized = SanitizeCompilerPath(view);

    // Try to match all sources
    for (size_t i = 0; i < physicalSources.size(); i++) {
        PhysicalSource *source = physicalSources[i];
        if (source->filename == sanitized) {
            return static_cast<uint32_t>(i);
        }
    }

    // Not found
    return InvalidSpvId;
}

SpvSourceMap::PhysicalSource *SpvSourceMap::GetOrAllocate(const std::string_view &view, SpvId id) {
    if (sourceMappings.size() <= id) {
        sourceMappings.resize(id + 1, InvalidSpvId);
    }

    for (PhysicalSource *source: physicalSources) {
        if (source->filename == view) {
            return source;
        }
    }

    for (uint32_t i = 0; i < physicalSources.size(); i++) {
        PhysicalSource *source = physicalSources[i];

        if (source->filename == view) {
            sourceMappings[id] = i;
            return source;
        }
    }

    sourceMappings[id] = static_cast<uint32_t>(physicalSources.size());
    return physicalSources.emplace_back(new (allocators) PhysicalSource);
}

SpvSourceMap::PhysicalSource *SpvSourceMap::GetOrAllocate(const std::string_view &view) {
    for (PhysicalSource *source: physicalSources) {
        if (source->filename == view) {
            return source;
        }
    }

    return physicalSources.emplace_back(new (allocators) PhysicalSource);
}

std::string_view SpvSourceMap::GetLine(uint32_t fileIndex, uint32_t line) const {
    const PhysicalSource *source = physicalSources.at(fileIndex);

    // TODO: Maybe this could be optimized

    // Find appropriate fragment
    for (const Fragment &fragment: source->fragments) {
        // Empty fragment?
        if (!fragment.source.length()) {
            continue;
        }

        // Within bounds?
        if (line >= fragment.lineStart && line < fragment.lineStart + fragment.lineOffsets.size()) {
            uint32_t offset = fragment.lineOffsets.at(line - fragment.lineStart);

            // End?
            if (offset == fragment.source.size()) {
                return {};
            }
            
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

    return physicalSources[0]->filename;
}

std::string_view SpvSourceMap::GetSourceFilename(uint32_t fileUID) const {
    return physicalSources.at(fileUID)->filename;
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
    const PhysicalSource *source = physicalSources[fileUID];

    // Not null terminated
    uint64_t length = 0;

    // Current line
    uint32_t line = 0;

    // Sum length
    for (const Fragment &fragment: source->fragments) {
        // Account for empty lines between fragments (required for 1:1 lineup)
        if (line < fragment.lineStart) {
            uint32_t missing = fragment.lineStart - line;
            length += missing;
            line += missing;
        }

        length += fragment.source.length();

        // Advance line count if not empty
        if (fragment.source.length()) {
            line += static_cast<uint32_t>(fragment.lineOffsets.size());
        }
    }

    return length;
}

void SpvSourceMap::FillCombinedSource(uint32_t fileUID, char *buffer) const {
    const PhysicalSource *source = physicalSources[fileUID];

    // Current line
    uint32_t line = 0;

    // Copy memory
    for (const Fragment &fragment: source->fragments) {
        // Account for empty lines between fragments (required for 1:1 lineup)
        if (line < fragment.lineStart) {
            uint32_t missing = fragment.lineStart - line;
            std::fill_n(buffer, missing, '\n');
            buffer += missing;
            line += missing;
        }

        std::memcpy(buffer, fragment.source.data(), fragment.source.length() * sizeof(char));
        buffer += fragment.source.length();

        // Advance line count if not empty
        if (fragment.source.length()) {
            line += static_cast<uint32_t>(fragment.lineOffsets.size());
        }
    }
}
