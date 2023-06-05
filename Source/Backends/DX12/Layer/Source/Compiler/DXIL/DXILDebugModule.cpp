#include <Backends/DX12/Compiler/DXIL/DXILDebugModule.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMHeader.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordStringView.h>

// Common
#include <Common/FileSystem.h>

DXILDebugModule::DXILDebugModule(const Allocators &allocators)
    : scan(allocators),
      sourceFragments(allocators),
      instructionMetadata(allocators),
      metadata(allocators),
      thinTypes(allocators),
      thinValues(allocators),
      allocators(allocators) { }

static std::string SanitizeCompilerPath(const std::string_view& view) {
    std::string path = SanitizePath(view);

    // Remove dangling delims
    if (path.ends_with("\\")) {
        path.erase(path.end());
    }

    // OK
    return path;
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
    // Postfix
    scan.SetDebugPostfix(".debug");

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
    value.thinType = record.Op32(0);

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
            case LLVMReservedBlock::UseList:
                break;
            case LLVMReservedBlock::Metadata:
                ParseMetadata(child);
                break;
            case LLVMReservedBlock::MetadataAttachment:
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
        uint32_t anchor = static_cast<uint32_t>(thinValues.size());

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
                metadata.sourceAssociation.column = record.OpAs<uint32_t>(1) - 1;

                // Has scope?
                if (uint32_t scope = record.OpAs<uint32_t>(2); scope) {
                    metadata.sourceAssociation.fileUID = static_cast<uint16_t>(GetLinearFileUID(scope - 1));
                }

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

    // Value anchor
    uint32_t anchor = static_cast<uint32_t>(metadata.size());

    // Preallocate
    metadata.resize(metadata.size() + block->records.size());

    // Visit records
    for (size_t i = 0; i < block->records.size(); i++) {
        const LLVMRecord &record = block->records[i];

        // Setup md
        Metadata& md = metadata[anchor + i];
        md.type = static_cast<LLVMMetadataRecord>(record.id);

        // Handle record
        switch (md.type) {
            default: {
                // Ignored
                break;
            }

            case LLVMMetadataRecord::Name: {
                // Set name
                recordName = LLVMRecordStringView(record, 0);

                // Validate next
                ASSERT(i + 1 != block->records.size(), "Expected succeeding metadata record");
                ASSERT(block->records[i + 1].Is(LLVMMetadataRecord::NamedNode), "Succeeding record to Name must be NamedNode");
                break;
            }

            case LLVMMetadataRecord::SubProgram: {
                md.subProgram.fileMdId = static_cast<uint32_t>(record.Op(4));
                break;
            }

            case LLVMMetadataRecord::LexicalBlock: {
                md.lexicalBlock.fileMdId = static_cast<uint32_t>(record.Op(2));
                break;
            }

            case LLVMMetadataRecord::LexicalBlockFile: {
                md.lexicalBlockFile.fileMdId = static_cast<uint32_t>(record.Op(2));
                break;
            }

            case LLVMMetadataRecord::Namespace: {
                md._namespace.fileMdId = static_cast<uint32_t>(record.Op(2));
                break;
            }

            case LLVMMetadataRecord::CompileUnit: {
                md.compileUnit.fileMdId = static_cast<uint32_t>(record.Op(2));
                break;
            }

            case LLVMMetadataRecord::File: {
                md.file.linearFileUID = static_cast<uint32_t>(sourceFragments.size());

                // Create fragment
                SourceFragment& fragment = sourceFragments.emplace_back(allocators);

                // Copy filename
                LLVMRecordStringView filename(block->records[record.Op(1) - 1], 0);
                fragment.filename.resize(filename.Length());
                filename.Copy(fragment.filename.data());

                // Cleanup
                fragment.filename = SanitizeCompilerPath(fragment.filename);
                break;
            }

            case LLVMMetadataRecord::NamedNode: {
                ParseNamedMetadata(block, anchor, record, recordName);
                break;
            }
        }
    }
}

void DXILDebugModule::ParseNamedMetadata(LLVMBlock* block, uint32_t anchor, const LLVMRecord &record, const struct LLVMRecordStringView& name) {
    switch (name.GetHash()) {
        case CRC64("dx.source.contents"): {
            if (name != "dx.source.contents") {
                return;
            }

            // Parse all files
            for (uint32_t i = 0; i < record.opCount; i++) {
                ParseContents(block, static_cast<uint32_t>(record.Op(i)));
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

void DXILDebugModule::ParseContents(LLVMBlock* block, uint32_t fileMdId) {
    const LLVMRecord& record = block->records[fileMdId];

    // Get strings
    LLVMRecordStringView filename(block->records[record.Op(0) - 1], 0);
    LLVMRecordStringView contents(block->records[record.Op(1) - 1], 0);

    // Target fragment
    SourceFragment* fragment = FindOrCreateSourceFragment(filename);

    // Already filled by another preprocessed segment?
    if (!fragment->lineOffsets.empty()) {
        return;
    }
    
    // May not exist
    if (!fragment) {
        ASSERT(false, "Unassociated file");
        return;
    }

    // Last known offset
    uint64_t lastSourceOffset = 0;

    // Append initial fragment
    fragment->lineOffsets.push_back(0);

    // Summarize the line offsets
    for (uint32_t i = 0; i < contents.Length(); i++) {
        if (contents[i] == '#') {
            constexpr const char *kLineDirective = "#line";

            // Start of directive
            uint32_t directiveStart = i;

            // Is line directive?
            if (!contents.StartsWithOffset(i, kLineDirective)) {
                continue;
            }

            // Consume last offset
            ASSERT(!fragment->lineOffsets.empty(), "No offset to consume");
            fragment->lineOffsets.pop_back();

            // Eat until number
            while (i < contents.Length() && !isdigit(contents[i])) {
                i++;
            }

            // Copy offset
            char offsetBuffer[255];
            contents.CopyUntilTerminated(i, offsetBuffer, sizeof(offsetBuffer) / sizeof(char), [](char ch) { return std::isdigit(ch); });

            // Parse line offset
            const uint32_t offset = atoi(offsetBuffer);

            // Eat until start of string
            while (i < contents.Length() && contents[i] != '"') {
                i++;
            }

            // Eat "
            i++;

            // Eat until end of string
            const uint32_t start = i;
            while (i < contents.Length() && contents[i] != '"') {
                i++;
            }

            // Copy offset
            auto fileChunk = ALLOCA_ARRAY(char, i - start + 1);
            contents.SubStrTerminated(start, i, fileChunk);

            // Get filename
            std::string file = SanitizeCompilerPath(fileChunk);

            // Eat until next line
            while (i < contents.Length() && contents[i] != '\n') {
                i++;
            }

            // Deduce length
            size_t fragmentLength = directiveStart - lastSourceOffset;

            // Copy contents
            size_t contentOffset = fragment->contents.length();
            fragment->contents.resize(contentOffset + fragmentLength);
            contents.SubStr(lastSourceOffset, directiveStart, fragment->contents.data() + contentOffset);
            
            // Extend fragments
            fragment = FindOrCreateSourceFragment(file);

            // Append expected newlines to new fragment
            for (size_t j = fragment->lineOffsets.size(); j < offset; j++) {
                ASSERT(fragment->lineOffsets.empty() || fragment->lineOffsets.back() <= static_cast<uint32_t>(fragment->contents.size()), "Invalid offset");
                fragment->lineOffsets.push_back(static_cast<uint32_t>(fragment->contents.size()));
                fragment->contents.push_back('\n');
            }

            // New offset
            lastSourceOffset = i;
        }

        else if (contents[i] == '\n') {
            auto localOffset = static_cast<uint32_t>(fragment->contents.size() + (i - lastSourceOffset));
            ASSERT(fragment->lineOffsets.back() <= localOffset, "Invalid offset");
            fragment->lineOffsets.push_back(localOffset);
        }
    }

    // Pending last fragment?
    if (contents.Length() > lastSourceOffset) {
        // Deduce length
        size_t fragmentLength = contents.Length() - lastSourceOffset;

        // Copy contents
        size_t contentOffset = fragment->contents.length();
        fragment->contents.resize(contentOffset + fragmentLength);
        contents.SubStr(lastSourceOffset, contents.Length(), fragment->contents.data() + contentOffset);
    }
}

std::string_view DXILDebugModule::GetFilename() {
    if (sourceFragments.empty()) {
        return {};
    }

    return sourceFragments[0].filename;
}

std::string_view DXILDebugModule::GetSourceFilename(uint32_t fileUID) {
    return sourceFragments.at(fileUID).filename;
}

uint32_t DXILDebugModule::GetFileCount() {
    return static_cast<uint32_t>(sourceFragments.size());
}

uint64_t DXILDebugModule::GetCombinedSourceLength(uint32_t fileUID) const {
    return sourceFragments.at(fileUID).contents.length();
}

void DXILDebugModule::FillCombinedSource(uint32_t fileUID, char *buffer) const {
    const SourceFragment& fragment = sourceFragments.at(fileUID);
    std::memcpy(buffer, fragment.contents.data(), fragment.contents.length());
}

uint32_t DXILDebugModule::GetLinearFileUID(uint32_t scopeMdId) {
    Metadata& md = metadata[scopeMdId];

    // Handle scope
    uint32_t fileMdId;
    switch (md.type) {
        default:
            ASSERT(false, "Unexpected scope id");
            return 0;
        case LLVMMetadataRecord::SubProgram: {
            fileMdId = md.subProgram.fileMdId;
            break;
        }
        case LLVMMetadataRecord::LexicalBlock: {
            fileMdId = md.lexicalBlock.fileMdId;
            break;
        }
        case LLVMMetadataRecord::LexicalBlockFile: {
            fileMdId = md.lexicalBlockFile.fileMdId;
            break;
        }
        case LLVMMetadataRecord::Namespace: {
            fileMdId = md._namespace.fileMdId;
            break;
        }
        case LLVMMetadataRecord::CompileUnit: {
            fileMdId = md.compileUnit.fileMdId;
            break;
        }
    }

    // Get file uid
    Metadata& fileMd = metadata[fileMdId - 1];
    ASSERT(fileMd.type == LLVMMetadataRecord::File, "Unexpected node");

    // OK
    return fileMd.file.linearFileUID;
}

DXILDebugModule::SourceFragment *DXILDebugModule::FindOrCreateSourceFragment(const LLVMRecordStringView &view) {
    // Copy to temporary string
    std::string filename;
    filename.resize(view.Length());
    view.Copy(filename.data());

    // Cleanup
    filename = SanitizeCompilerPath(filename);

    // Check on filename
    return FindOrCreateSourceFragment(filename);
}

DXILDebugModule::SourceFragment * DXILDebugModule::FindOrCreateSourceFragment(const std::string_view &view) {
    // Find fragment
    for (SourceFragment& candidate : sourceFragments) {
        if (candidate.filename == view) {
            return &candidate;
        }
    }

    // The fragment doesn't exist, likely indicating that it was not used in the shader
    SourceFragment& fragment = sourceFragments.emplace_back(allocators);

    // Assign filename
    fragment.filename = std::move(view);
    
    // OK
    return &fragment;
}
