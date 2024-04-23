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

#include <Backends/Vulkan/Compiler/SpvPhysicalBlockScan.h>

SpvPhysicalBlockScan::SpvPhysicalBlockScan(IL::Program &program) : program(program) {

}

bool SpvPhysicalBlockScan::Scan(const uint32_t *const code, uint32_t count) {
    const uint32_t *offset = code;
    const uint32_t *end = code + count;

    // Must be able to accommodate header
    if (count < 5) {
        return false;
    }

    // Consume header
    header.magic = *(offset++);
    header.version = *(offset++);
    header.generator = *(offset++);
    header.bound = *(offset++);
    header.reserved = *(offset++);

    // Validate magic header
    if (header.magic != SpvMagicNumber) {
        return false;
    }

    // Decode version
    majorVersion = (header.version >> 16) & 0xFF;
    minorVersion = (header.version >>  8) & 0xFF;

    // Set identifier bound
    program.GetIdentifierMap().SetBound(header.bound);

    // Initialize all blocks
    for (Section &section: sections) {
        section.physicalBlock.source.programBegin = code;
        section.physicalBlock.source.programEnd = code + count;
        section.physicalBlock.source.span.begin = IL::InvalidOffset;
        section.physicalBlock.source.span.end = 0;
    }

    // Current block type
    SpvPhysicalBlockType currentBlock = SpvPhysicalBlockType::Capability;

    // Parse instruction stream
    while (offset < end) {
        auto *instruction = reinterpret_cast<const SpvInstruction *>(offset);

        // Get category
        SpvOp op = instruction->GetOp();
        SpvPhysicalBlockType type = GetSpvBlockType(op);

        // Must be ordered
        if (type == SpvPhysicalBlockType::Count || static_cast<uint32_t>(type) < static_cast<uint32_t>(currentBlock)) {
            return false;
        }

        // Number of words in the instruction
        uint32_t wordCount = instruction->GetWordCount();

        // Current offset
        uint32_t source = static_cast<uint32_t>(offset - code);

        // Skip further function definitions
        if (type == SpvPhysicalBlockType::Function) {
            Section& section = sections[static_cast<uint32_t>(type)];
            section.physicalBlock.source.span.begin = source;
            section.physicalBlock.source.span.end = count;
            break;
        }

        // Set span
        Section& section = sections[static_cast<uint32_t>(type)];
        section.physicalBlock.source.span.begin = std::min(section.physicalBlock.source.span.begin, source);
        section.physicalBlock.source.span.end = std::max(section.physicalBlock.source.span.end, source + wordCount);

        // Next
        offset += wordCount;
    }

    // Allocate streams
    for (uint32_t i = 0; i < static_cast<uint32_t>(SpvPhysicalBlockType::Count); i++) {
        Section& section = sections[i];

        // Initialize stream
        section.physicalBlock.stream = SpvStream(code);

        // Create source
        if (section.physicalBlock.source.span.begin == IL::InvalidOffset) {
            section.physicalBlock.source.code = nullptr;
            section.physicalBlock.source.end = 0;
            continue;
        }

        uint32_t wordCount = section.physicalBlock.source.span.end - section.physicalBlock.source.span.begin;

        // Set segment
        section.physicalBlock.source.code = (code + section.physicalBlock.source.span.begin);
        section.physicalBlock.source.end = section.physicalBlock.source.code + wordCount;

        // Append any data
        //   ? Functions are handled separately
        if (i < static_cast<uint32_t>(SpvPhysicalBlockType::Function)) {
            section.physicalBlock.stream.AppendData(code + section.physicalBlock.source.span.begin, wordCount);
        }
    }

    // OK
    return true;
}

uint64_t SpvPhysicalBlockScan::GetModuleWordCount() const {
    uint64_t wordCount = 0;

    // Header
    wordCount += sizeof(header) / sizeof(uint32_t);

    // Sections
    for (const Section& section : sections) {
        wordCount += section.physicalBlock.stream.GetWordCount();
    }

    return wordCount;
}

void SpvPhysicalBlockScan::Stitch(SpvStream &out) {
    // Preallocate storage
    out.Reserve(GetModuleWordCount());

    // Write header
    out.AppendData(&header, sizeof(header) / sizeof(uint32_t));

    // Append all sections
    for (uint32_t i = 0; i < static_cast<uint32_t>(SpvPhysicalBlockType::Count); i++) {
        const Section& section = sections[i];
        out.AppendData(section.physicalBlock.stream.GetData(), static_cast<uint32_t>(section.physicalBlock.stream.GetWordCount()));
    }
}

void SpvPhysicalBlockScan::CopyTo(SpvPhysicalBlockScan &out) {
    out.header = header;
    out.majorVersion = majorVersion;
    out.minorVersion = minorVersion;

    for (uint32_t i = 0; i < static_cast<uint32_t>(SpvPhysicalBlockType::Count); i++) {
        out.sections[i] = sections[i];
    }
}
