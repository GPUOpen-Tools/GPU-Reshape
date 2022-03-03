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
        uint32_t source = offset - code;

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
        if (section.physicalBlock.source.span.begin != IL::InvalidOffset) {
            uint32_t wordCount = section.physicalBlock.source.span.end - section.physicalBlock.source.span.begin;

            section.physicalBlock.source.code = (code + section.physicalBlock.source.span.begin);
            section.physicalBlock.source.end = section.physicalBlock.source.code + wordCount;

            // Append any data
            //   ? Functions are handled separately
            if (i < static_cast<uint32_t>(SpvPhysicalBlockType::Function)) {
                section.physicalBlock.stream.AppendData(code + section.physicalBlock.source.span.begin, wordCount);
            }
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
        out.AppendData(section.physicalBlock.stream.GetData(), section.physicalBlock.stream.GetWordCount());
    }
}

void SpvPhysicalBlockScan::CopyTo(SpvPhysicalBlockScan &out) {
    out.header = header;

    for (uint32_t i = 0; i < static_cast<uint32_t>(SpvPhysicalBlockType::Count); i++) {
        out.sections[i] = sections[i];
    }
}
