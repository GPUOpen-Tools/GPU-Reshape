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

#include <Backends/DX12/Compiler/DXIL/DXILDebugModule.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMHeader.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordStringView.h>
#include <Backends/DX12/Compiler/DXBC/Blocks/DXBCPhysicalBlockShaderSourceInfo.h>

// Common
#include <Common/FileSystem.h>

DXILDebugModule::DXILDebugModule(const Allocators &allocators, const DXBCPhysicalBlockShaderSourceInfo& shaderSourceInfo)
    : scan(allocators),
      sourceFragments(allocators),
      instructionMetadata(allocators),
      metadata(allocators),
      thinTypes(allocators),
      thinValues(allocators),
      allocators(allocators),
      shaderSourceInfo(shaderSourceInfo) { }

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

    // Assume source info block over embedded sources
    if (!shaderSourceInfo.sourceFiles.empty()) {
        CreateFragmentsFromSourceBlock();
    }

    // Do we need to resolve?
    if (isContentsUnresolved) {
        RemapLineScopes();
    }

    // OK
    return true;
}

void DXILDebugModule::RemapLineScopes() {
    for (InstructionMetadata& md : instructionMetadata) {
        // Unmapped or invalid?
        if (md.sourceAssociation.fileUID == UINT16_MAX ||
            md.sourceAssociation.fileUID >= sourceFragments.size()) {
            continue;
        }

        // The parent fragment
        SourceFragment& targetFragment = sourceFragments.at(md.sourceAssociation.fileUID);

        // Current directive
        SourceFragmentDirective candidateDirective;

        // Check all preprocessed fragments
        for (const SourceFragmentDirective& directive : targetFragment.preprocessedDirectives) {
            if (directive.directiveLineOffset > md.sourceAssociation.line) {
                break;
            }

            // Consider candidate
            candidateDirective = directive;
        }

        // No match? (Part of the primary fragment)
        if (candidateDirective.fileUID == UINT16_MAX) {
            continue;
        }

        // Offset within the directive file
        const uint32_t intraDirectiveOffset = md.sourceAssociation.line - candidateDirective.directiveLineOffset;

        // Remap the association
        md.sourceAssociation.fileUID = candidateDirective.fileUID;
        md.sourceAssociation.line = candidateDirective.fileLineOffset + intraDirectiveOffset; 
    }
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
    value.kind = ThinValueKind::Function;

    // Set type
    value.thinType = record.Op32(0);

    // Inherit non-semantic from type
    value.bIsNonSemantic |= thinTypes.at(value.thinType).bIsNonSemantic;
}

void DXILDebugModule::ParseFunction(LLVMBlock *block) {
    // Keep current head
    const size_t valueHead = thinValues.size();
    
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
                ASSERT(called.kind == ThinValueKind::Function, "Mismatched thin type");

                // Ignore non-semantic instructions from cross-referencing
                if (!called.bIsNonSemantic) {
                    instructionMetadata.emplace_back();
                }
                
                // Allocate return value if need be
                if (!thinTypes[called.thinType].function.isVoidReturn) {
                    thinValues.emplace_back();
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

    // Reset head, value indices reset after function blocks
    thinValues.resize(valueHead);
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
        case GRS_CRC32("dx.source.contents"): {
            if (name != "dx.source.contents") {
                return;
            }

            // If the source info block has any contents, ignore embedded
            if (!shaderSourceInfo.sourceFiles.empty()) {
                return;
            }

            // A single file either indicates that there's a single file, or, that the contents are unresolved
            // f.x. line directives that need to be mapped
            isContentsUnresolved = (record.opCount == 1u);

            // Parse all files
            for (uint32_t i = 0; i < record.opCount; i++) {
                ParseContents(block, static_cast<uint32_t>(record.Op(i)));
            }
            break;
        }

        case GRS_CRC32("dx.source.mainFileName"): {
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

    // Target fragment which may be derived
    SourceFragment* fragment = FindOrCreateSourceFragmentSanitized(filename);

    // Fragments are stored contiguously, just keep the uid
    uint32_t targetUID = fragment->uid;

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

    // Current target fragment line offset
    uint32_t targetLineOffset = 0;

    /** TODO: This is such a mess! I'll clean this up when it's matured a bit. */

    // Append initial line
    fragment->lineOffsets.push_back(0);

    // Summarize the line offsets
    for (uint32_t i = 0; i < contents.Length(); i++) {
        constexpr const char *kLineDirective = "#line";

        // Target newline?
        if (contents[i] == '\n') {
            targetLineOffset++;
        }

        // Start of directive
        uint32_t directiveStart = i;

        // Is line directive?
        if (!contents.StartsWithOffset(i, kLineDirective)) {
            continue;
        }

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

        // Directive newline
        targetLineOffset++;

        // Do not include the directive new-line, search backwards
        uint32_t lastSourceEnd = directiveStart;
        while (lastSourceEnd > 0 && contents[lastSourceEnd] != '\n') {
            lastSourceEnd--;
        }

        // Deduce length
        size_t fragmentLength = lastSourceEnd >= lastSourceOffset ? lastSourceEnd - lastSourceOffset : 0ull;

        // Copy contents
        size_t contentOffset = fragment->contents.length();
        fragment->contents.resize(contentOffset + fragmentLength);
        contents.SubStr(lastSourceOffset, lastSourceEnd, fragment->contents.data() + contentOffset);

        // Summarize line endings
        for (size_t j = contentOffset; j < fragment->contents.size(); j++) {
            if (fragment->contents[j] == '\n') {
                fragment->lineOffsets.push_back(static_cast<uint32_t>(j + 1));
            }
        }

        // Extend fragments
        fragment = FindOrCreateSourceFragment(file);

        // Append initial line
        if (fragment->lineOffsets.empty()) {
            fragment->lineOffsets.push_back(0);
        }

        // Append expected newlines to new fragment
        for (size_t j = fragment->lineOffsets.size(); j < offset; j++) {
            fragment->contents.push_back('\n'); 
            fragment->lineOffsets.push_back(static_cast<uint32_t>(fragment->contents.size()));
        }

        // New offset
        lastSourceOffset = i + 1;

        // Keep track of in the target
        sourceFragments.at(targetUID).preprocessedDirectives.push_back(SourceFragmentDirective {
            .fileUID = fragment->uid,
            .fileLineOffset = offset - 1u,
            .directiveLineOffset = targetLineOffset
        });
    }

    // Pending last fragment?
    if (contents.Length() > lastSourceOffset) {
        // Deduce length
        size_t fragmentLength = contents.Length() - lastSourceOffset;

        // Copy contents
        size_t contentOffset = fragment->contents.length();
        fragment->contents.resize(contentOffset + fragmentLength);
        contents.SubStr(lastSourceOffset, contents.Length(), fragment->contents.data() + contentOffset);

        // Summarize line endings
        for (size_t j = contentOffset; j < fragment->contents.size(); j++) {
            if (fragment->contents[j] == '\n') {
                fragment->lineOffsets.push_back(static_cast<uint32_t>(j + 1));
            }
        }
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

void DXILDebugModule::CreateFragmentsFromSourceBlock() {
    // Block contents should never need resolving 
    isContentsUnresolved = false;

    // Fill all files
    for (const DXBCPhysicalBlockShaderSourceInfo::SourceFile& file : shaderSourceInfo.sourceFiles) {
        SourceFragment* fragment = FindOrCreateSourceFragmentSanitized(file.filename);

        // Block contents shouldn't require any preprocessing
        // TODO: Consider backing storage, avoids needless copies
        fragment->contents = file.contents;

        // Initial line
        fragment->lineOffsets.push_back(0);
        
        // Summarize remaining line endings
        for (size_t i = 0; i < fragment->contents.size(); i++) {
            if (fragment->contents[i] == '\n') {
                fragment->lineOffsets.push_back(static_cast<uint32_t>(i + 1));
            }
        }
    }
}

DXILDebugModule::SourceFragment *DXILDebugModule::FindOrCreateSourceFragmentSanitized(const LLVMRecordStringView &view) {
    // Copy to temporary string
    std::string filename;
    filename.resize(view.Length());
    view.Copy(filename.data());

    // Cleanup
    filename = SanitizeCompilerPath(filename);

    // Check on filename
    return FindOrCreateSourceFragment(filename);
}

DXILDebugModule::SourceFragment * DXILDebugModule::FindOrCreateSourceFragmentSanitized(const std::string_view &view) {
    // Cleanup
    std::string filename = SanitizeCompilerPath(view);

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
    fragment.uid = static_cast<uint16_t>(sourceFragments.size()) - 1u;

    // Assign filename
    fragment.filename = std::move(view);
    
    // OK
    return &fragment;
}
