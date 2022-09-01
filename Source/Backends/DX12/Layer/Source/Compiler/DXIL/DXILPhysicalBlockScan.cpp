#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockScan.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBitStreamReader.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBitStreamWriter.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMHeader.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBlock.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecord.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMAbbreviation.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBlockMetadata.h>

// Common
#include <Common/Allocators.h>

/// Dump bit stream to file?
///   Useful with llvm-bcanalyzer
#define DXIL_DUMP_BITSTREAM 1

// Special includes
#if DXIL_DUMP_BITSTREAM
// Layer
#   include <Backends/DX12/Compiler/DXIL/LLVM/LLVMPrettyPrint.h>

// Common
#   include <Common/FileSystem.h>

// Std
#   include <fstream>
#endif // DXIL_DUMP_BITSTREAM

// Validation mode for 1:1 read / writes
#define DXIL_VALIDATE_MIRROR 1

/*
 * The LLVM bit-stream specification is easy to understand, that is once you've already implemented it.
 *
 * LLVM Specification
 *   https://llvm.org/docs/BitCodeFormat.html#module-code-version-record
 * */

DXILPhysicalBlockScan::DXILPhysicalBlockScan(const Allocators &allocators) : allocators(allocators) {

}

void DXILPhysicalBlockScan::SetBlockFilter(uint64_t shlBitMask) {
    shlBlockFilter = shlBitMask;
}

bool DXILPhysicalBlockScan::Scan(const void *byteCode, uint64_t byteLength) {
    auto bcHeader = static_cast<const DXILHeader *>(byteCode);

    // Keep header
    header = *bcHeader;

    // Must be DXIL
    if (header.identifier != 'LIXD') {
        return false;
    }

    // Construct bit stream
    //   ? Bit streams begin from the identifier
    LLVMBitStreamReader stream(reinterpret_cast<const uint8_t *>(&bcHeader->identifier) + header.codeOffset, header.codeSize);

    // Dump?
#if DXIL_DUMP_BITSTREAM
    // Write stream to immediate path
    std::ofstream out(GetIntermediateDebugPath() / "LLVM.bstream", std::ios_base::binary);
    out.write(reinterpret_cast<const char *>(&bcHeader->identifier) + header.codeOffset, header.codeSize);
    out.close();
#endif

    // Validate bit stream
    if (!stream.ValidateAndConsume()) {
        return false;
    }

    // Must start with a sub-block
    if (stream.FixedEnum<LLVMReservedAbbreviation>(2) != LLVMReservedAbbreviation::EnterSubBlock) {
        return false;
    }

    // Parse block hierarchy
    if (ScanEnterSubBlock(stream, &root) != ScanResult::OK || stream.IsError()) {
        return false;
    }

    // Missing records?
#if 0
    if (!stream.IsEOS()) {
        ASSERT(false, "Reached end of scan with pending stream data");
        return false;
    }
#endif // 0

    // Dump?
#if DXIL_DUMP_BITSTREAM
    // Write stream to immediate path
    std::ofstream outScan(GetIntermediateDebugPath() / "LLVM.scan.txt");
    PrettyPrint(outScan, &root);
    outScan.close();
#endif

    // OK
    return true;
}

DXILPhysicalBlockScan::ScanResult DXILPhysicalBlockScan::ScanEnterSubBlock(LLVMBitStreamReader &stream, LLVMBlock *block) {
    /*
     * LLVM Specification
     *   [ENTER_SUBBLOCK, blockidvbr8, newabbrevlenvbr4, <align32bits>, blocklen_32]
     * */

    // Read identifier
    block->id = stream.VBR<uint32_t>(8);

    // Set uid
    block->uid = uidCounter++;

    // Any metadata?
    if (auto it = metadataLookup.find(block->id); it != metadataLookup.end()) {
        block->metadata = it->second;
    }

    // Read abbreviation size
    block->abbreviationSize = stream.VBR<uint32_t>(4);

    // Align32
    stream.AlignDWord();

    // Read number of dwords
    block->blockLength = stream.Fixed<uint32_t>();

    // Id zero indicates BLOCKINFO
    if (block->id == 0) {
        return ScanBlockInfo(stream, block, block->abbreviationSize);
    }

    // Part of filter?
    if (shlBlockFilter != UINT64_MAX && !(shlBlockFilter & (1ull << block->id))) {
        // Just skip the contents
        stream.Skip(block->blockLength - sizeof(uint32_t));
        return ScanResult::OK;
    }

    // Scan all abbreviations
    for (;;) {
        // Read id
        auto abbreviationId = stream.Fixed<uint32_t>(block->abbreviationSize);

        // Stop current block?
        if (abbreviationId == static_cast<uint32_t>(LLVMReservedAbbreviation::EndBlock)) {
            stream.AlignDWord();
            break;
        }

        // Handle identifier
        switch (abbreviationId) {
            default: {
                if (ScanRecord(stream, block, abbreviationId) != ScanResult::OK) {
                    return ScanResult::Error;
                }
                break;
            }

            case static_cast<uint32_t>(LLVMReservedAbbreviation::EnterSubBlock): {
                // Add element
                block->elements.Add(LLVMBlockElement(LLVMBlockElementType::Block, block->blocks.Size()));

                // Scan child tree
                if (ScanEnterSubBlock(stream, block->blocks.Add(new(Allocators{}) LLVMBlock)) != ScanResult::OK) {
                    return ScanResult::Error;
                }
                break;
            }

            case static_cast<uint32_t>(LLVMReservedAbbreviation::DefineAbbreviation): {
                if (ScanAbbreviation(stream, block) != ScanResult::OK) {
                    return ScanResult::Error;
                }
                break;
            }

            case static_cast<uint32_t>(LLVMReservedAbbreviation::UnabbreviatedRecord): {
                if (ScanUnabbreviatedRecord(stream, block) != ScanResult::OK) {
                    return ScanResult::Error;
                }
                break;
            }
        }
    }

    // OK
    return ScanResult::OK;
}

DXILPhysicalBlockScan::ScanResult DXILPhysicalBlockScan::ScanAbbreviation(LLVMBitStreamReader &stream, LLVMBlock *block) {
    /*
     * LLVM Specification
     *   [DEFINE_ABBREV, numabbrevopsvbr5, abbrevop0, abbrevop1, …]
     * */

    // Add element
    block->elements.Add(LLVMBlockElement(LLVMBlockElementType::Abbreviation, block->abbreviations.Size()));

    // Create new abbreviation
    LLVMAbbreviation &abbreviation = block->abbreviations.Add();

    // Get number of ops
    auto opCount = stream.VBR<uint32_t>(5);

    // Preallocate
    abbreviation.parameters.Resize(opCount);

    // Scan all ops
    for (uint32_t i = 0; i < opCount; i++) {
        LLVMAbbreviationParameter &parameter = abbreviation.parameters[i];

        // Literal abbr?
        auto isLiteral = stream.Fixed<uint32_t>(1);

        // Scan abbr
        if (isLiteral) {
            /*
             * LLVM Specification
             *   [11, litvaluevbr8]
             * */

            parameter.encoding = LLVMAbbreviationEncoding::Literal;
            parameter.value = stream.VBR<uint64_t>(8);
        } else {
            /*
             * LLVM Specification
             *   [01, encoding3]
             *   [01, encoding3, valuevbr5]
             * */

            // Scan encoding
            parameter.encoding = stream.FixedEnum<LLVMAbbreviationEncoding>(3);

            // Read value if present
            switch (parameter.encoding) {
                default:
                    break;
                case LLVMAbbreviationEncoding::Fixed:
                case LLVMAbbreviationEncoding::VBR:
                    parameter.value = stream.VBR<uint64_t>(5);
                    break;
            }
        }
    }

    // OK
    return ScanResult::OK;
}

DXILPhysicalBlockScan::ScanResult DXILPhysicalBlockScan::ScanUnabbreviatedRecord(LLVMBitStreamReader &stream, LLVMBlock *block) {
    /*
     * LLVM Specification
     *   [UNABBREV_RECORD, codevbr6, numopsvbr6, op0vbr6, op1vbr6, …]
     * */

    // Add element
    block->elements.Add(LLVMBlockElement(LLVMBlockElementType::Record, block->records.Size()));

    // Add record
    LLVMRecord &record = block->records.Add();

    // Get id
    record.id = stream.VBR<uint32_t>(6);

    // Get number of operands
    record.opCount = stream.VBR<uint32_t>(6);

    // Allocate
    record.ops = new(Allocators{}) uint64_t[record.opCount];

    // Scan all ops
    for (uint32_t i = 0; i < record.opCount; i++) {
        record.ops[i] = stream.VBR<uint64_t>(6);
    }

    // OK
    return ScanResult::OK;
}

DXILPhysicalBlockScan::ScanResult DXILPhysicalBlockScan::ScanBlockInfo(LLVMBitStreamReader &stream, LLVMBlock *block, uint32_t abbreviationSize) {
    // Current metadata
    LLVMBlockMetadata *metadata{nullptr};

    // Scan all abbreviations
    for (;;) {
        // Read id
        auto abbreviationId = stream.Fixed<uint32_t>(abbreviationSize);

        // Stop current block?
        if (abbreviationId == static_cast<uint32_t>(LLVMReservedAbbreviation::EndBlock)) {
            stream.AlignDWord();
            break;
        }

        // Handle identifier
        switch (abbreviationId) {
            default: {
                ASSERT(false, "Unexpected abbreviation id in BLOCKINFO");
                return ScanResult::Error;
            }

            case static_cast<uint32_t>(LLVMReservedAbbreviation::DefineAbbreviation): {
                ASSERT(metadata, "Abbreviation in BLOCKINFO without SETBID");

                if (ScanAbbreviation(stream, metadata) != ScanResult::OK) {
                    return ScanResult::Error;
                }
                break;
            }

            case static_cast<uint32_t>(LLVMReservedAbbreviation::UnabbreviatedRecord): {
                LLVMBlockMetadata* current = metadata;

                // May change the block
                if (ScanUnabbreviatedInfoRecord(stream, &metadata) != ScanResult::OK) {
                    return ScanResult::Error;
                }

                // Add as child if new BID
                if (current != metadata) {
                    block->blocks.Add(metadata);
                }
                break;
            }
        }
    }

    // OK
    return ScanResult::OK;
}

DXILPhysicalBlockScan::ScanResult DXILPhysicalBlockScan::ScanUnabbreviatedInfoRecord(LLVMBitStreamReader &stream, LLVMBlockMetadata **metadata) {
    /*
     * LLVM Specification
     *   [UNABBREV_RECORD, codevbr6, numopsvbr6, op0vbr6, op1vbr6, …]
     * */

    // Add record
    LLVMRecord record;

    // Get id
    record.id = stream.VBR<uint32_t>(6);

    // Get number of operands
    record.opCount = stream.VBR<uint32_t>(6);

    // Allocate
    record.ops = new(Allocators{}) uint64_t[record.opCount];

    // Scan all ops
    for (uint32_t i = 0; i < record.opCount; i++) {
        record.ops[i] = stream.VBR<uint64_t>(6);
    }

    // Handle type
    switch (record.id) {
        default: {
            break;
        }
        case static_cast<uint32_t>(LLVMBlockInfoRecord::SetBID): {
            // Allocate meta
            auto *meta = new(Allocators{}) LLVMBlockMetadata;

            // Set id
            meta->id = record.ops[0];

            // Assign lookup
            ASSERT(record.opCount == 1, "Unexpected record count");
            metadataLookup[meta->id] = meta;

            // Set new metadata
            *metadata = meta;

            // OK
            return ScanResult::OK;
        }
        case static_cast<uint32_t>(LLVMBlockInfoRecord::BlockName): {
            // TODO: Parse name
            break;
        }
        case static_cast<uint32_t>(LLVMBlockInfoRecord::SetRecordName): {
            // TODO: Parse record name
            break;
        }
    }

    // Must have metadata
    (*metadata)->records.Add(record);

    // OK
    return ScanResult::OK;
}

DXILPhysicalBlockScan::ScanResult DXILPhysicalBlockScan::ScanRecord(LLVMBitStreamReader &stream, LLVMBlock *block, uint32_t encodedAbbreviationId) {
    /*
     * LLVM Specification
     *   Abbreviation IDs 4 and above are defined by the stream itself, and specify an abbreviated record encoding.
     * */

    // Check validity
    if (encodedAbbreviationId < 4) {
        ASSERT(false, "Invalid record entered");
        return ScanResult::Error;
    }

    // Add element
    block->elements.Add(LLVMBlockElement(LLVMBlockElementType::Record, block->records.Size()));

    // Add record
    LLVMRecord &record = block->records.Add();

    // To index
    uint32_t abbreviationIndex = encodedAbbreviationId - 4;

    // Get abbreviation
    const LLVMAbbreviation *abbreviation{nullptr};

    // Does this block have an associated BLOCKINFO?
    if (block->metadata) {
        /*
         * LLVM Specification
         *   Any abbreviations defined in a BLOCKINFO record for the particular block type receive IDs first, in order, followed by any abbreviations defined within the block itself.
         *   Abbreviated data records reference this ID to indicate what abbreviation they are invoking.
         * */

        // Part of the BLOCKINFO's set?
        if (abbreviationIndex < block->metadata->abbreviations.Size()) {
            abbreviation = &block->metadata->abbreviations[abbreviationIndex];

            // Set as metadata
            record.abbreviation.type = LLVMRecordAbbreviationType::BlockMetadata;
            record.abbreviation.abbreviationId = abbreviationIndex;
        } else {
            abbreviationIndex -= block->metadata->abbreviations.Size();
        }
    }

    // No info or beyond the info abbreviations?
    if (!abbreviation) {
        abbreviation = &block->abbreviations[abbreviationIndex];

        // Set as local
        record.abbreviation.type = LLVMRecordAbbreviationType::BlockLocal;
        record.abbreviation.abbreviationId = abbreviationIndex;
    }

    // Scan id
    record.id = ScanTrivialAbbreviationParameter(stream, abbreviation->parameters[0]);

    // Flush
    recordOperandCache.clear();

    // Early reserve
    if (recordOperandCache.capacity() < abbreviation->parameters.Size() - 1) {
        recordOperandCache.reserve(abbreviation->parameters.Size() - 1);
    }

    // Scan all ops
    for (uint32_t i = 1; i < abbreviation->parameters.Size(); i++) {
        const LLVMAbbreviationParameter &parameter = abbreviation->parameters[i];

        // Handle encoding
        switch (parameter.encoding) {
            default: {
                ASSERT(false, "Unknown encoding");
                return ScanResult::Error;
            }

                /* Trivial types */
            case LLVMAbbreviationEncoding::Literal:
            case LLVMAbbreviationEncoding::Fixed:
            case LLVMAbbreviationEncoding::VBR:
            case LLVMAbbreviationEncoding::Char6: {
                recordOperandCache.push_back(ScanTrivialAbbreviationParameter(stream, parameter));
                break;
            }

                /* Array */
            case LLVMAbbreviationEncoding::Array: {
                /*
                 * LLVM Specification
                *    When reading an array in an abbreviated record, the first integer is a vbr6 that indicates the array length, followed by the encoded elements of the array.
                 *   An array may only occur as the last operand of an abbreviation (except for the one final operand that gives the array’s type).
                 * */

                // Get contained type
                const LLVMAbbreviationParameter &contained = abbreviation->parameters[++i];

                // Get array count
                auto count = stream.VBR<uint64_t>(6);

                // Scan all elements
                for (uint32_t elementIndex = 0; elementIndex < count; elementIndex++) {
                    recordOperandCache.push_back(ScanTrivialAbbreviationParameter(stream, contained));
                }

                // Must be end
                ASSERT(i == abbreviation->parameters.Size() - 1, "Array contained type not last abbreviation parameter");
                break;
            }

                /* Blob */
            case LLVMAbbreviationEncoding::Blob: {
                /*
                 * LLVM Specification
                *    This field is emitted as a vbr6, followed by padding to a 32-bit boundary (for alignment) and an array of 8-bit objects.
                 *    The array of bytes is further followed by tail padding to ensure that its total length is a multiple of 4 bytes.
                 * */

                // Read size
                record.blobSize = stream.VBR<uint64_t>(6);

                // Align
                stream.AlignDWord();

                // Assign blob
                record.blob = stream.GetSafeData();

                // Skip blob (takes byte count)
                stream.Skip(record.blobSize);
                break;
            }
        }
    }

    // Set op count
    record.opCount = recordOperandCache.size();

    // Allocate final records
    record.ops = new(Allocators{}) uint64_t[recordOperandCache.size()];
    std::memcpy(record.ops, recordOperandCache.data(), sizeof(uint64_t) * recordOperandCache.size());

    // OK
    return ScanResult::OK;
}

uint64_t DXILPhysicalBlockScan::ScanTrivialAbbreviationParameter(LLVMBitStreamReader &stream, const LLVMAbbreviationParameter &parameter) {
    switch (parameter.encoding) {
        default:
        ASSERT(false, "Unexpected encoding");
            return 0;

            /* Handle cases */
        case LLVMAbbreviationEncoding::Literal: {
            return parameter.value;
        }
        case LLVMAbbreviationEncoding::Fixed: {
            return stream.Fixed<uint64_t>(parameter.value);
        }
        case LLVMAbbreviationEncoding::VBR: {
            return stream.VBR<uint64_t>(parameter.value);
        }
        case LLVMAbbreviationEncoding::Char6: {
            return stream.Char6();
        }
    }
}

#if DXIL_VALIDATE_MIRROR
static void ValidateBlock(const LLVMBlock *lhs, const LLVMBlock *rhs) {
    // Validate properties
    ASSERT(lhs->id == rhs->id, "Id mismatch");
    ASSERT(lhs->abbreviationSize == rhs->abbreviationSize, "Abbreviation size mismatch");
    ASSERT(lhs->blocks.Size() == rhs->blocks.Size(), "Block count mismatch");
    ASSERT(lhs->abbreviations.Size() == rhs->abbreviations.Size(), "Abbreviation count mismatch");
    ASSERT(lhs->records.Size() == rhs->records.Size(), "Record count mismatch");
    ASSERT(lhs->elements.Size() == rhs->elements.Size(), "Element count mismatch");

    for (size_t elementIdx = 0; elementIdx < lhs->elements.Size(); elementIdx++) {
        const LLVMBlockElement& elementLhs = lhs->elements[elementIdx];
        const LLVMBlockElement& elementRhs = rhs->elements[elementIdx];

        ASSERT(elementLhs.type == elementRhs.type, "Element type mismatch");

        if (elementLhs.Is(LLVMBlockElementType::Block)) {
            const LLVMBlock* blockLhs = lhs->blocks[elementLhs.id];
            const LLVMBlock* blockRhs = rhs->blocks[elementRhs.id];
            ValidateBlock(blockLhs, blockRhs);
        } else if (elementLhs.Is(LLVMBlockElementType::Abbreviation)) {
            const LLVMAbbreviation& abbreviationLhs = lhs->abbreviations[elementLhs.id];
            const LLVMAbbreviation& abbreviationRhs = rhs->abbreviations[elementRhs.id];

            // Validate size
            ASSERT(abbreviationLhs.parameters.Size() == abbreviationRhs.parameters.Size(), "Abbreviation parameter count mismatch");

            // Validate parameters
            for (size_t parameterIdx = 0; parameterIdx < abbreviationLhs.parameters.Size(); parameterIdx++) {
                const LLVMAbbreviationParameter& paramLhs = abbreviationLhs.parameters[parameterIdx];
                const LLVMAbbreviationParameter& paramRhs = abbreviationRhs.parameters[parameterIdx];

                // Validate properties
                ASSERT(paramLhs.encoding == paramRhs.encoding, "Abbreviation parameter encoding mismatch");
                ASSERT(paramLhs.value == paramRhs.value, "Abbreviation parameter value mismatch");
            }
        } else if (elementLhs.Is(LLVMBlockElementType::Record)) {
            const LLVMRecord& recordLhs = lhs->records[elementLhs.id];
            const LLVMRecord& recordRhs = rhs->records[elementRhs.id];

            // Validate properties
            ASSERT(recordLhs.id == recordRhs.id, "Record id mismatch");
            ASSERT(recordLhs.abbreviation.type == recordRhs.abbreviation.type, "Record encoded abbreviation type mismatch");
            ASSERT(recordLhs.blobSize == recordRhs.blobSize, "Record blob size mismatch");
            ASSERT(recordLhs.opCount == recordRhs.opCount, "Record op count mismatch");

            // Validate blob if present
            if (recordLhs.blobSize) {
                ASSERT(!std::memcmp(recordLhs.blob, recordRhs.blob, recordLhs.blobSize), "Record blob mismatch");
            }

            // Validate all ops
            for (size_t opIdx = 0; opIdx < recordLhs.opCount; opIdx++) {
                ASSERT(recordLhs.ops[opIdx] == recordRhs.ops[opIdx], "Record op mismatch");
            }
        }
    }

    // Any metadata?
    if (lhs->metadata) {
        ASSERT(rhs->metadata, "Block metadata mismatch");
        ValidateBlock(lhs->metadata, rhs->metadata);
    }
}
#endif // DXIL_VALIDATE_MIRROR

void DXILPhysicalBlockScan::Stitch(DXStream &out) {
    // Dump?
#if DXIL_DUMP_BITSTREAM
    // Write stream to immediate path
    std::ofstream outScan(GetIntermediateDebugPath() / "LLVM.stitch.txt");
    PrettyPrint(outScan, &root);
    outScan.close();
#endif // DXIL_DUMP_BITSTREAM

    // Write back header
    uint64_t headerOffset = out.Append(header);

#if DXIL_VALIDATE_MIRROR
    size_t validationOffset = out.GetByteSize();
#endif // DXIL_VALIDATE_MIRROR

    // Standard writer
    LLVMBitStreamWriter stream(out);

    // Add header
    stream.AddHeaderValidation();

    // Enter block
    stream.FixedEnum<LLVMReservedAbbreviation>(LLVMReservedAbbreviation::EnterSubBlock, 2);

    // Write root block
    WriteSubBlock(stream, &root);

    // Close up any loose words
    stream.Close();

    // End offset
    uint64_t endOffset = out.GetOffset();

    // Total length
    uint64_t byteLength = endOffset - headerOffset;

    // Patch header
    auto* stitchHeader = out.GetMutableDataAt<DXILHeader>(headerOffset);
    stitchHeader->dwordCount = static_cast<uint32_t>(byteLength / sizeof(uint32_t));
    stitchHeader->codeSize = static_cast<uint32_t>(byteLength - sizeof(DXILHeader));

    // Dump?
#if DXIL_DUMP_BITSTREAM
    // Write stream to immediate path
    std::ofstream outStitch(GetIntermediateDebugPath() / "LLVM.stitch.bstream", std::ios_base::binary);
    outStitch.write(reinterpret_cast<const char*>(out.GetData()) + validationOffset, out.GetByteSize() - validationOffset);
    outStitch.close();
#endif

    // Validate written stream by re-reading it,
#if DXIL_VALIDATE_MIRROR
    LLVMBitStreamReader reader(out.GetData() + validationOffset, out.GetByteSize() - validationOffset);

    // Validate bit stream
    ASSERT(reader.ValidateAndConsume(), "Validation failed");

    // Block for validation
    LLVMBlock mirrorBlock;

    // Must start with a sub-block
    ASSERT(reader.FixedEnum<LLVMReservedAbbreviation>(2) == LLVMReservedAbbreviation::EnterSubBlock, "First block must be EnterSubBlock");

    // Parse block hierarchy
    ASSERT(ScanEnterSubBlock(reader, &mirrorBlock) == ScanResult::OK && !reader.IsError(), "Failed to parse hierarchy");

    // Validate away!
    ValidateBlock(&root, &mirrorBlock);
#endif // DXIL_VALIDATE_MIRROR
}

void DXILPhysicalBlockScan::CopyTo(DXILPhysicalBlockScan &out) {
    out.header = header;
    out.metadataLookup = metadataLookup;

    // Copy root block
    CopyBlock(&root, out.root);
}

void DXILPhysicalBlockScan::CopyBlock(const LLVMBlock *block, LLVMBlock &out) {
    // Immutable data
    out.id = block->id;
    out.uid = block->uid;
    out.abbreviationSize = block->abbreviationSize;
    out.blockLength = block->blockLength;
    out.metadata = block->metadata;
    out.records = block->records;
    out.abbreviations = block->abbreviations;
    out.elements = block->elements;

    // Mutable data
    for (const LLVMBlock *child: block->blocks) {
        auto copy = new(Allocators{}) LLVMBlock;
        CopyBlock(child, *copy);
        out.blocks.Add(copy);
    }
}

DXILPhysicalBlockScan::WriteResult DXILPhysicalBlockScan::WriteSubBlock(LLVMBitStreamWriter &stream, const LLVMBlock *block) {
    /*
     * LLVM Specification
     *   [ENTER_SUBBLOCK, blockidvbr8, newabbrevlenvbr4, <align32bits>, blocklen_32]
     * */

    // Write identifier
    stream.VBR<uint32_t>(block->id, 8);

    // Write abbreviation size
    stream.VBR<uint32_t>(block->abbreviationSize, 4);

    // Align32
    stream.AlignDWord();

    // Write number of dwords
    LLVMBitStreamWriter::Position lengthPos = stream.Fixed<uint32_t>(0);

    // Id zero indicates BLOCKINFO
    if (block->id == 0) {
        return WriteBlockInfo(stream, block, lengthPos);
    }

    // Write according to element order
    for (const LLVMBlockElement& element : block->elements) {
        switch (static_cast<LLVMBlockElementType>(element.type)) {
            case LLVMBlockElementType::Block: {
                const LLVMBlock* child = block->blocks[element.id];

                // Header
                stream.Fixed<uint32_t>(static_cast<uint32_t>(LLVMReservedAbbreviation::EnterSubBlock), block->abbreviationSize);

                // Write it!
                if (WriteResult result = WriteSubBlock(stream, child); result != WriteResult::OK) {
                    return result;
                }
                break;
            }
            case LLVMBlockElementType::Abbreviation: {
                const LLVMAbbreviation& abbreviation = block->abbreviations[element.id];

                // Header
                stream.Fixed<uint32_t>(static_cast<uint32_t>(LLVMReservedAbbreviation::DefineAbbreviation), block->abbreviationSize);

                // Write it!
                if (WriteResult result = WriteAbbreviation(stream, abbreviation); result != WriteResult::OK) {
                    return result;
                }
                break;
            }
            case LLVMBlockElementType::Record: {
                const LLVMRecord& record = block->records[element.id];

                // Has abbreviation?
                WriteResult result;
                if (record.abbreviation.type != LLVMRecordAbbreviationType::None) {
                    /*
                     * LLVM Specification
                     *   Any abbreviations defined in a BLOCKINFO record for the particular block type receive IDs first, in order, followed by any abbreviations defined within the block itself.
                     *   Abbreviated data records reference this ID to indicate what abbreviation they are invoking.
                     * */

                    uint32_t encodedAbbreviationId = 4;

                    // Determine id from scanned type
                    switch (record.abbreviation.type) {
                        default: {
                            ASSERT(false, "Unexpected abbreviation type");
                            break;
                        }
                        case LLVMRecordAbbreviationType::BlockMetadata: {
                            encodedAbbreviationId += record.abbreviation.abbreviationId;
                            break;
                        }
                        case LLVMRecordAbbreviationType::BlockLocal: {
                            // Index after metadata abbreviations
                            if (block->metadata) {
                                encodedAbbreviationId += block->metadata->abbreviations.Size();
                            }

                            // Local indexc
                            encodedAbbreviationId += record.abbreviation.abbreviationId;
                            break;
                        }
                    }

                    // Write id
                    stream.Fixed<uint32_t>(encodedAbbreviationId, block->abbreviationSize);

                    // Write contents
                    result = WriteRecord(stream, block, record);
                } else {
                    // Write id
                    stream.Fixed<uint32_t>(static_cast<uint32_t>(LLVMReservedAbbreviation::UnabbreviatedRecord), block->abbreviationSize);

                    // Write contents
                    result = WriteUnabbreviatedRecord(stream, record);
                }

                // OK?
                if (result != WriteResult::OK) {
                    return result;
                }
                break;
            }
        }
    }

    // End block
    stream.Fixed<uint32_t>(static_cast<uint32_t>(LLVMReservedAbbreviation::EndBlock), block->abbreviationSize);

    // Must be aligned
    stream.AlignDWord();

    // Patch position, -1 to exclude the length itself
    stream.FixedPatch<uint32_t>(lengthPos, LLVMBitStreamWriter::Position::DWord(lengthPos, stream.Pos()) - 1);

    // OK
    return WriteResult::OK;
}

DXILPhysicalBlockScan::WriteResult DXILPhysicalBlockScan::WriteBlockInfo(LLVMBitStreamWriter &stream, const LLVMBlock *block, const LLVMBitStreamWriter::Position &lengthPos) {
    // Each metadata segment is exposed as a block
    for (const LLVMBlock* bid : block->blocks) {
        // Emit SetBID
        stream.Fixed<uint32_t>(static_cast<uint32_t>(LLVMReservedAbbreviation::UnabbreviatedRecord), block->abbreviationSize);
        stream.VBR<uint32_t>(static_cast<uint32_t>(LLVMBlockInfoRecord::SetBID), 6);

        // One operand [opCount, bid]
        stream.VBR<uint32_t>(1, 6);
        stream.VBR<uint32_t>(bid->id, 6);

        // Write all abbreviations
        for (const LLVMAbbreviation &abbreviation: bid->abbreviations) {
            stream.Fixed<uint32_t>(static_cast<uint32_t>(LLVMReservedAbbreviation::DefineAbbreviation), block->abbreviationSize);

            // Write it!
            if (WriteResult result = WriteAbbreviation(stream, abbreviation); result != WriteResult::OK) {
                return result;
            }
        }

        // Write all records
        for (const LLVMRecord &record: bid->records) {
            stream.Fixed<uint32_t>(static_cast<uint32_t>(LLVMReservedAbbreviation::UnabbreviatedRecord), block->abbreviationSize);

            // Write it!
            if (WriteResult result = WriteUnabbreviatedRecord(stream, record); result != WriteResult::OK) {
                return result;
            }
        }
    }

    // End block
    stream.Fixed<uint32_t>(static_cast<uint32_t>(LLVMReservedAbbreviation::EndBlock), block->abbreviationSize);

    // Must be aligned
    stream.AlignDWord();

    // Patch position, -1 to exclude the length itself
    stream.FixedPatch<uint32_t>(lengthPos, LLVMBitStreamWriter::Position::DWord(lengthPos, stream.Pos()) - 1);

    // OK
    return WriteResult::OK;
}

DXILPhysicalBlockScan::WriteResult DXILPhysicalBlockScan::WriteAbbreviation(LLVMBitStreamWriter &stream, const LLVMAbbreviation &abbreviation) {
    /*
     * LLVM Specification
     *   [DEFINE_ABBREV, numabbrevopsvbr5, abbrevop0, abbrevop1, …]
     * */

    // Write count
    stream.VBR<uint32_t>(abbreviation.parameters.Size(), 5);

    for (const LLVMAbbreviationParameter &parameter: abbreviation.parameters) {
        const bool isLiteral = parameter.encoding == LLVMAbbreviationEncoding::Literal;

        // Write literal state
        stream.Fixed<uint32_t>(isLiteral, 1);

        if (isLiteral) {
            /*
             * LLVM Specification
             *   [11, litvaluevbr8]
             * */

            stream.VBR<uint64_t>(parameter.value, 8);
        } else {
            /*
             * LLVM Specification
             *   [01, encoding3]
             *   [01, encoding3, valuevbr5]
             * */

            // Write encoding
            stream.FixedEnum<LLVMAbbreviationEncoding>(parameter.encoding, 3);

            // Write value
            switch (parameter.encoding) {
                default:
                    break;
                case LLVMAbbreviationEncoding::Fixed:
                case LLVMAbbreviationEncoding::VBR:
                    stream.VBR<uint64_t>(parameter.value, 5);
                    break;
            }
        }
    }

    // OK
    return WriteResult::OK;
}

DXILPhysicalBlockScan::WriteResult DXILPhysicalBlockScan::WriteUnabbreviatedRecord(LLVMBitStreamWriter &stream, const LLVMRecord &record) {
    /*
    * LLVM Specification
    *   [UNABBREV_RECORD, codevbr6, numopsvbr6, op0vbr6, op1vbr6, …]
    * */

    // Write id
    stream.VBR<uint32_t>(record.id, 6);

    // Write count
    stream.VBR<uint32_t>(record.opCount, 6);

    // Write all operands
    for (uint32_t i = 0; i < record.opCount; i++) {
        stream.VBR<uint64_t>(record.ops[i], 6);
    }

    // OK
    return WriteResult::OK;
}

DXILPhysicalBlockScan::WriteResult DXILPhysicalBlockScan::WriteRecord(LLVMBitStreamWriter &stream, const LLVMBlock *block, const LLVMRecord &record) {
    // Get abbreviation
    const LLVMAbbreviation *abbreviation{nullptr};

    // Determine from scanned type
    switch (record.abbreviation.type) {
        case LLVMRecordAbbreviationType::None:
            break;
        case LLVMRecordAbbreviationType::BlockMetadata:
            abbreviation = &block->metadata->abbreviations[record.abbreviation.abbreviationId];
            break;
        case LLVMRecordAbbreviationType::BlockLocal:
            abbreviation = &block->abbreviations[record.abbreviation.abbreviationId];
            break;
    }

    // Validate
    ASSERT(abbreviation, "Writing abbreviated record with no abbreviation");

    // Current operand offset
    uint32_t operandOffset{0};

    // Write all operands
    for (uint32_t i = 1; i < abbreviation->parameters.Size(); i++) {
        const LLVMAbbreviationParameter &parameter = abbreviation->parameters[i];

        // Handle encoding
        switch (parameter.encoding) {
            default: {
                ASSERT(false, "Unknown encoding");
                return WriteResult::Error;
            }

                /* Trivial types */
            case LLVMAbbreviationEncoding::Literal:
            case LLVMAbbreviationEncoding::Fixed:
            case LLVMAbbreviationEncoding::VBR:
            case LLVMAbbreviationEncoding::Char6: {
                if (WriteResult result = WriteTrivialAbbreviationParameter(stream, parameter, record.Op(operandOffset++)); result != WriteResult::OK) {
                    return result;
                }
                break;
            }

                /* Array */
            case LLVMAbbreviationEncoding::Array: {
                /*
                 * LLVM Specification
                *    When reading an array in an abbreviated record, the first integer is a vbr6 that indicates the array length, followed by the encoded elements of the array.
                 *   An array may only occur as the last operand of an abbreviation (except for the one final operand that gives the array’s type).
                 * */

                // Get contained type
                const LLVMAbbreviationParameter &contained = abbreviation->parameters[++i];

                // Always the last parameter
                const uint32_t count = record.opCount - operandOffset;

                // Write count
                stream.VBR<uint64_t>(count, 6);

                // Write all elements
                for (uint32_t elementIndex = 0; elementIndex < count; elementIndex++) {
                    if (WriteResult result = WriteTrivialAbbreviationParameter(stream, contained, record.Op(operandOffset++)); result != WriteResult::OK) {
                        return result;
                    }
                }
                break;
            }

                /* Blob */
            case LLVMAbbreviationEncoding::Blob: {
                /*
                 * LLVM Specification
                *    This field is emitted as a vbr6, followed by padding to a 32-bit boundary (for alignment) and an array of 8-bit objects.
                 *    The array of bytes is further followed by tail padding to ensure that its total length is a multiple of 4 bytes.
                 * */

                // Write size
                stream.VBR<uint64_t>(record.blobSize, 6);

                // Align to dword
                stream.AlignDWord();

                // Write out blob, size already aligned to dword
                ASSERT(record.blobSize % sizeof(uint32_t) == 0, "Blob size must dword aligned");
                stream.WriteDWord(record.blob, record.blobSize / sizeof(uint32_t));
                break;
            }
        }
    }

    // OK
    return WriteResult::OK;
}

DXILPhysicalBlockScan::WriteResult DXILPhysicalBlockScan::WriteTrivialAbbreviationParameter(LLVMBitStreamWriter &stream, const LLVMAbbreviationParameter &parameter, uint64_t op) {
    switch (parameter.encoding) {
        default:
        ASSERT(false, "Unexpected encoding");
            return WriteResult::Error;

            /* Handle cases */
        case LLVMAbbreviationEncoding::Literal: {
            // No writeback, already part of the abbreviation
            break;
        }
        case LLVMAbbreviationEncoding::Fixed: {
            stream.Fixed<uint64_t>(op, parameter.value);
            break;
        }
        case LLVMAbbreviationEncoding::VBR: {
            stream.VBR<uint64_t>(op, parameter.value);
            break;
        }
        case LLVMAbbreviationEncoding::Char6: {
            stream.Char6(op);
            break;
        }
    }

    // OK
    return WriteResult::OK;
}
