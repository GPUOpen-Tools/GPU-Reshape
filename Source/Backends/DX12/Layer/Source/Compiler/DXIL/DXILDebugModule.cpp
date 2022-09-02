#include <Backends/DX12/Compiler/DXIL/DXILDebugModule.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMHeader.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordStringView.h>

DXILDebugModule::DXILDebugModule(const Allocators &allocators) : scan(allocators) {

}

DXSourceAssociation DXILDebugModule::GetSourceAssociation(uint32_t codeOffset) {
    if (codeOffset >= instructionMetadata.size()) {
        return {};
    }

    return instructionMetadata[codeOffset].sourceAssociation;
}

std::string_view DXILDebugModule::GetLine(uint32_t fileUID, uint32_t line) {
    // Safeguard file
    if (fileUID >= sourceFragments.size()) {
        return {};
    }

    SourceFragment& fragment = sourceFragments.at(fileUID);

    // Safeguard line
    if (line >= fragment.lineOffsets.size()) {
        return {};
    }

    // Base offset
    uint32_t base = fragment.lineOffsets.at(line);

    // Get view
    if (line == fragment.lineOffsets.size() - 1) {
        return std::string_view(fragment.contents.data() + base, fragment.contents.length() - base);
    } else {
        return std::string_view(fragment.contents.data() + base, fragment.lineOffsets.at(line + 1) - base);
    }
}

bool DXILDebugModule::Parse(const void *byteCode, uint64_t byteLength) {
    // Disable debug writing
    scan.SetEnableDebugging(false);

    // Scan data
    if (!scan.Scan(byteCode, byteLength)) {
        return false;
    }

    // Get root
    LLVMBlock &root = scan.GetRoot();

    // Naive value head
    thinValues.reserve(128);

    // Pre-parse all types for local fetching
    for (LLVMBlock *block: root.blocks) {
        switch (static_cast<LLVMReservedBlock>(block->id)) {
            default:
                // Handled later
                break;
            case LLVMReservedBlock::Type:
                ParseTypes(block);
                break;
        }
    }

    // Visit all records
    for (LLVMRecord &record: root.records) {
        switch (static_cast<LLVMModuleRecord>(record.id)) {
            default:
                break;
            case LLVMModuleRecord::GlobalVar:
                thinValues.emplace_back();
                break;
            case LLVMModuleRecord::Function:
                ParseModuleFunction(record);
                break;
            case LLVMModuleRecord::Alias:
                thinValues.emplace_back();
                break;
        }
    }


    // Visit all blocks
    for (LLVMBlock *block: root.blocks) {
        switch (static_cast<LLVMReservedBlock>(block->id)) {
            default:
                break;
            case LLVMReservedBlock::Constants:
                ParseConstants(block);
                break;
            case LLVMReservedBlock::Function:
                ParseFunction(block);
                break;
            case LLVMReservedBlock::Metadata:
                ParseMetadata(block);
                break;
        }
    }

    // OK
    return true;
}

void DXILDebugModule::ParseTypes(LLVMBlock *block) {
    uint32_t typeCounter{0};

    // Visit type records
    for (const LLVMRecord &record: block->records) {
        if (record.Is(LLVMTypeRecord::NumEntry)) {
            thinTypes.resize(record.ops[0]);
            continue;
        }

        if (record.Is(LLVMTypeRecord::StructName)) {
            continue;
        }

        ThinType& type = thinTypes.at(typeCounter++);
        type.type = record.As<LLVMTypeRecord>();

        switch (type.type) {
            default: {
                break;
            }
            case LLVMTypeRecord::MetaData: {
                type.bIsNonSemantic = true;
                break;
            }
            case LLVMTypeRecord::Function: {
                // Void return type?
                type.function.isVoidReturn = thinTypes.at(record.Op(1)).type == LLVMTypeRecord::Void;

                // Inherit non-semantic from parameters
                for (uint32_t i = 2; i < record.opCount; i++) {
                    type.bIsNonSemantic |= thinTypes.at(record.Op(i)).bIsNonSemantic;
                }
                break;
            }
        }
    }
}

void DXILDebugModule::ParseModuleFunction(const LLVMRecord& record) {
    ThinValue& value = thinValues.emplace_back();

    // Set type
    value.thinType = record.Op(0);

    // Inherit non-semantic from type
    value.bIsNonSemantic |= thinTypes.at(value.thinType).bIsNonSemantic;
}

void DXILDebugModule::ParseFunction(LLVMBlock *block) {
    for(LLVMBlock* child : block->blocks) {
        switch (child->As<LLVMReservedBlock>()) {
            default:
                ASSERT(false, "Invalid block");
                break;
            case LLVMReservedBlock::ValueSymTab:
                break;
            case LLVMReservedBlock::Metadata:
                ParseMetadata(child);
                break;
            case LLVMReservedBlock::Constants:
                ParseConstants(child);
                break;
        }
    }

    // Pending metadata
    InstructionMetadata metadata;

    for (const LLVMBlockElement& element : block->elements) {
        if (!element.Is(LLVMBlockElementType::Record)) {
            continue;
        }

        // Get record
        const LLVMRecord& record = block->records[element.id];

        // Current anchor
        uint32_t anchor = thinValues.size();

        // Handle record
        switch (static_cast<LLVMFunctionRecord>(record.id)) {
            default: {
                // Result value?
                if (HasValueAllocation(record.As<LLVMFunctionRecord>(), record.opCount)) {
                    thinValues.emplace_back();
                }

                // Add metadata and consume
                instructionMetadata.emplace_back();
                break;
            }

            case LLVMFunctionRecord::InstCall:
            case LLVMFunctionRecord::InstCall2: {
                const ThinValue& called = thinValues.at(anchor - record.Op(3));

                // Allocate return value if need be
                if (!thinTypes[called.thinType].function.isVoidReturn) {
                    thinValues.emplace_back();
                }

                if (!called.bIsNonSemantic) {
                    instructionMetadata.emplace_back();
                }
                break;
            }

            case LLVMFunctionRecord::DebugLOC:
            case LLVMFunctionRecord::DebugLOC2: {
                metadata.sourceAssociation.fileUID = 0;
                metadata.sourceAssociation.line = record.OpAs<uint32_t>(0) - 1;
                metadata.sourceAssociation.column = record.OpAs<uint32_t>(1);

                if (instructionMetadata.size()) {
                    instructionMetadata.back() = metadata;
                }
                break;
            }

            case LLVMFunctionRecord::DebugLOCAgain: {
                // Repush pending
                if (instructionMetadata.size()) {
                    instructionMetadata.back() = metadata;
                }
                break;
            }
        }
    }
}

void DXILDebugModule::ParseConstants(LLVMBlock *block) {
    for (LLVMRecord &record: block->records) {
        if (record.Is(LLVMConstantRecord::SetType)) {
            continue;
        }

        thinValues.emplace_back();
    }
}

void DXILDebugModule::ParseMetadata(LLVMBlock *block) {
    // Current name
    LLVMRecordStringView recordName;

    // Visit records
    for (size_t i = 0; i < block->records.Size(); i++) {
        const LLVMRecord &record = block->records[i];

        // Handle record
        switch (static_cast<LLVMMetadataRecord>(record.id)) {
            default: {
                // Ignored
                break;
            }

            case LLVMMetadataRecord::Name: {
                // Set name
                recordName = LLVMRecordStringView(record, 0);

                // Validate next
                ASSERT(i + 1 != block->records.Size(), "Expected succeeding metadata record");
                ASSERT(block->records[i + 1].Is(LLVMMetadataRecord::NamedNode), "Succeeding record to Name must be NamedNode");
                break;
            }

            case LLVMMetadataRecord::NamedNode: {
                ParseNamedMetadata(block, record, recordName);
                break;
            }
        }
    }
}

void DXILDebugModule::ParseNamedMetadata(LLVMBlock* block, const LLVMRecord &record, const struct LLVMRecordStringView& name) {
    switch (name.GetHash()) {
        case CRC64("dx.source.contents"): {
            if (name != "dx.source.contents") {
                return;
            }

            // Get list
            ASSERT(record.opCount == 1, "Expected a single value for dx.source.contents");
            const LLVMRecord &list = block->records[record.Op(0)];

            // Get all filenames
            for (uint32_t i = 0; i < list.opCount; i += 2) {
                LLVMRecordStringView filename(block->records[list.Op(i) - 1], 0);
                LLVMRecordStringView contents(block->records[list.Op(i + 1) - 1], 0);

                // Create fragment
                SourceFragment& fragment = sourceFragments.emplace_back();

                // Allocate
                fragment.filename.resize(filename.Length());
                fragment.contents.resize(contents.Length());

                // Copy data
                filename.Copy(fragment.filename.data());
                contents.Copy(fragment.contents.data());

                // Count number of line endings
                uint32_t lineEndingCount = std::count(fragment.contents.begin(), fragment.contents.end(), '\n');
                fragment.lineOffsets.reserve(lineEndingCount + 1);

                // First line
                fragment.lineOffsets.push_back(0);

                // Summarize line offsets
                for (size_t offset = 0; offset < fragment.contents.length(); offset++) {
                    if (fragment.contents[offset] == '\n') {
                        fragment.lineOffsets.push_back(offset + 1);
                    }
                }
            }
            break;
        }

        case CRC64("dx.source.mainFileName"): {
            if (name != "dx.source.mainFileName") {
                return;
            }
            break;
        }
    }
}
