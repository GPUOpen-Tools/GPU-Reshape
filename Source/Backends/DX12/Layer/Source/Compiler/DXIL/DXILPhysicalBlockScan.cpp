#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockScan.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBitStream.h>
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
// Common
#   include <Common/FileSystem.h>

// Std
#   include <fstream>
#endif

/*
 * The LLVM bit-stream specification is easy to understand, that is once you've already implemented it.
 *
 * LLVM Specification
 *   https://llvm.org/docs/BitCodeFormat.html#module-code-version-record
 * */

DXILPhysicalBlockScan::DXILPhysicalBlockScan(const Allocators &allocators) : allocators(allocators) {

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
    LLVMBitStream stream(reinterpret_cast<const uint8_t *>(&bcHeader->identifier) + header.codeOffset, header.codeSize);

    // Dump?
#if DXIL_DUMP_BITSTREAM
    // Write stream to immediate path
    std::ofstream out(GetIntermediateDebugPath() / "LLVM.bstream", std::ios_base::binary);
    out.write(reinterpret_cast<const char*>(&bcHeader->identifier) + header.codeOffset, header.codeSize);
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

    // OK
    return true;
}

DXILPhysicalBlockScan::ScanResult DXILPhysicalBlockScan::ScanEnterSubBlock(LLVMBitStream &stream, LLVMBlock *block) {
    /*
     * LLVM Specification
     *   [ENTER_SUBBLOCK, blockidvbr8, newabbrevlenvbr4, <align32bits>, blocklen_32]
     * */

    // Read identifier
    block->id = stream.VBR<uint32_t>(8);

    // Any metadata?
    if (auto it = metadataLookup.find(block->id); it != metadataLookup.end()) {
        block->metadata = it->second;
    }

    // Read abbreviation size
    auto abbreviationSize = stream.VBR<uint32_t>(4);

    // Align32
    stream.AlignDWord();

    // Read number of dwords
    auto blockLength = stream.Fixed<uint32_t>();

    // Id zero indicates BLOCKINFO
    if (block->id == 0) {
        return ScanBlockInfo(stream, block, abbreviationSize);
    }

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
                if (ScanRecord(stream, block, abbreviationId) != ScanResult::OK) {
                    return ScanResult::Error;
                }
                break;
            }

            case static_cast<uint32_t>(LLVMReservedAbbreviation::EnterSubBlock): {
                if (ScanEnterSubBlock(stream, block->blocks.Add(new (Allocators{}) LLVMBlock)) != ScanResult::OK) {
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

DXILPhysicalBlockScan::ScanResult DXILPhysicalBlockScan::ScanAbbreviation(LLVMBitStream &stream, LLVMBlock* block) {
    /*
     * LLVM Specification
     *   [DEFINE_ABBREV, numabbrevopsvbr5, abbrevop0, abbrevop1, …]
     * */

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

DXILPhysicalBlockScan::ScanResult DXILPhysicalBlockScan::ScanUnabbreviatedRecord(LLVMBitStream &stream, LLVMBlock* block) {
    /*
     * LLVM Specification
     *   [UNABBREV_RECORD, codevbr6, numopsvbr6, op0vbr6, op1vbr6, …]
     * */

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

DXILPhysicalBlockScan::ScanResult DXILPhysicalBlockScan::ScanBlockInfo(LLVMBitStream &stream, LLVMBlock *block, uint32_t abbreviationSize) {
    // Current metadata
    LLVMBlockMetadata* metadata{nullptr};

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
                if (ScanUnabbreviatedInfoRecord(stream, &metadata) != ScanResult::OK) {
                    return ScanResult::Error;
                }
                break;
            }
        }
    }

    // OK
    return ScanResult::OK;
}

DXILPhysicalBlockScan::ScanResult DXILPhysicalBlockScan::ScanUnabbreviatedInfoRecord(LLVMBitStream &stream, LLVMBlockMetadata** metadata) {
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
        case static_cast<uint32_t>(LLVMBlockInfoRecord::SetBID): {
            // Allocate meta
            auto* meta = new (Allocators{}) LLVMBlockMetadata;

            // Assign lookup
            ASSERT(record.opCount == 1, "Unexpected record count");
            metadataLookup[record.ops[0]] = meta;

            // Set new metadata
            *metadata = meta;
            break;
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

    // Must have metdata
    (*metadata)->records.Add(record);

    // OK
    return ScanResult::OK;
}

DXILPhysicalBlockScan::ScanResult DXILPhysicalBlockScan::ScanRecord(LLVMBitStream &stream, LLVMBlock* block, uint32_t encodedAbbreviationId) {
    /*
     * LLVM Specification
     *   Abbreviation IDs 4 and above are defined by the stream itself, and specify an abbreviated record encoding.
     * */

    // Check validity
    if (encodedAbbreviationId < 4) {
        ASSERT(false, "Invalid record entered");
        return ScanResult::Error;
    }

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
        } else {
            abbreviationIndex -= block->metadata->abbreviations.Size();
        }
    }

    // No info or beyond the info abbreviations?
    if (!abbreviation) {
        abbreviation = &block->abbreviations[abbreviationIndex];
    }

    // Add record
    LLVMRecord &record = block->records.Add();

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

uint64_t DXILPhysicalBlockScan::ScanTrivialAbbreviationParameter(LLVMBitStream &stream, const LLVMAbbreviationParameter &parameter) {
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

void DXILPhysicalBlockScan::Stitch(DXStream &out) {

}
