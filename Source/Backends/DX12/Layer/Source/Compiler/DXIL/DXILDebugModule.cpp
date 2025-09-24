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
#include <Backends/DX12/Compiler/DXIL/DXILModule.h>

// Backend
#include <Backend/IL/Function.h>

// Common
#include <Common/FileSystem.h>

DXILDebugModule::DXILDebugModule(const Allocators &allocators, DXILModule* module, const DXBCPhysicalBlockShaderSourceInfo &shaderSourceInfo)
    : scan(allocators),
      sourceFragments(allocators),
      functionMetadata(allocators),
      valueStrings(allocators.Tag(kAllocModuleDXILSymbols)),
      valueAllocations(allocators.Tag(kAllocModuleDXILSymbols)),
      metadata(allocators),
      thinTypes(allocators),
      thinValues(allocators),
      thinFunctions(allocators),
      allocators(allocators),
      module(module),
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

DXSourceAssociation DXILDebugModule::GetSourceAssociation(const IL::Function* function, uint32_t codeOffset) {
    DXILPhysicalBlockTable& table = module->GetTable();

    // Get the linked index
    uint32_t index = table.function.GetNonPrototypeFunctionIndex(function->GetID());

    FunctionMetadata& md = functionMetadata[index];
    if (codeOffset >= md.instructionMetadata.size()) {
        return {};
    }

    return md.instructionMetadata[codeOffset].sourceAssociation;
}

std::span<DXInstructionAssociation> DXILDebugModule::GetInstructionAssociations(uint16_t fileUID, uint32_t line) {
    // Find all associations
    auto it = instructionAssociations.find(DXSourceAssociation{
        .fileUID = fileUID,
        .line = line
    }.GetKey());

    // May not exist
    if (it == instructionAssociations.end()) {
        return {};
    }

    // Get view
    return std::span(it->second.set.data(), it->second.set.size());
}

DXDwardInfo DXILDebugModule::GetDwarfInfo(const IL::Function *function, uint32_t codeOffset) {
    DXILPhysicalBlockTable& table = module->GetTable();

    // Get the linked index
    uint32_t index = table.function.GetNonPrototypeFunctionIndex(function->GetID());

    // Get function
    FunctionMetadata& md = functionMetadata[index];
    if (codeOffset >= md.instructionMetadata.size()) {
        return {};
    }

    // May not have any debug info
    auto it = md.instructionDwarfInfos.find(codeOffset);
    if (it == md.instructionDwarfInfos.end()) {
        return {};
    }

    return DXDwardInfo {
        .name = it->second.name,
        .type = GetTypeFromDwarf(it->second.typeMdId),
        .values = it->second.values
    };
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
            case LLVMReservedBlock::ValueSymTab:
                ParseSymTab(block);
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

    // Populate reverse lookup
    CreateReverseAssociations();

    // OK
    return true;
}

void DXILDebugModule::RemapLineScopes() {
    for (FunctionMetadata& functionMd : functionMetadata) {
        for (InstructionMetadata& md : functionMd.instructionMetadata) {
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
}

void DXILDebugModule::CreateReverseAssociations() {
    DXILPhysicalBlockTable& table = module->GetTable();
    
    for (uint64_t linkIndex = 0; linkIndex < functionMetadata.size(); linkIndex++) {
        FunctionMetadata& functionMd = functionMetadata[linkIndex];

        // Get the declaration
        const DXILFunctionDeclaration *functionDeclaration = table.function.GetFunctionDeclarationFromIndex(static_cast<uint32_t>(linkIndex));

        // Create key -> record lookups
        for (uint64_t recordIndex = 0; recordIndex < functionMd.instructionMetadata.size(); recordIndex++) {
            InstructionMetadata& md = functionMd.instructionMetadata[recordIndex];
            
            // Unmapped or invalid?
            if (md.sourceAssociation.fileUID == UINT16_MAX || md.sourceAssociation.fileUID >= sourceFragments.size()) {
                continue;
            }

            // Get the set
            InstructionAssociationSet &instructionSet = instructionAssociations[DXSourceAssociation{
                .fileUID = md.sourceAssociation.fileUID,
                .line = md.sourceAssociation.line
            }.GetKey()];

            // Add to set
            instructionSet.set.push_back(DXInstructionAssociation {
                .functionId = functionDeclaration->functionId,
                .codeOffset = static_cast<uint32_t>(recordIndex)
            });
        }
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

                // Number of parameters
                type.function.parameterCount = record.opCount - 2;

                // Allocate types
                type.function.parameterTypes = blockAllocator.AllocateArray<uint32_t>(type.function.parameterCount);

                // Inherit non-semantic from parameters
                for (uint32_t i = 2; i < record.opCount; i++) {
                    type.function.parameterTypes[i - 2] = static_cast<uint32_t>(record.Op(i));
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

    // Prototype?
    if (!record.Op32(2)) {
        thinFunctions.push_back(ThinFunction {
            .thinType = value.thinType
        });
    }

    // Inherit non-semantic from type
    value.bIsNonSemantic |= thinTypes.at(value.thinType).bIsNonSemantic;
}

void DXILDebugModule::ParseFunction(LLVMBlock *block) {
    // Keep current head
    const size_t valueHead = thinValues.size();

    // Get type, appears in linkage order
    const ThinFunction& function = thinFunctions[functionLinkIndex++];

    // Create new metadata entry
    FunctionMetadata& functionMd = functionMetadata.emplace_back();

    // Create value per parameter
    for (uint32_t i = 0; i < thinTypes[function.thinType].function.parameterCount; i++) {
        ThinValue& value = thinValues.emplace_back();
        value.kind = ThinValueKind::Parameter;
    }
    
    for(LLVMBlock* child : block->blocks) {
        switch (child->As<LLVMReservedBlock>()) {
            default:
                ASSERT(false, "Invalid block");
                break;
            case LLVMReservedBlock::ValueSymTab:
                ParseSymTab(child);
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

    /// Was the last instruction semantically relevant?
    bool isSemanticInstruction = false;

    /// Current source record offset, not debug
    uint32_t recordOffset = 0;

    for (uint32_t recordIdx = 0; recordIdx < static_cast<uint32_t>(block->records.size()); recordIdx++) {
        LLVMRecord &record = block->records[recordIdx];

        // Current anchor
        uint32_t anchor = static_cast<uint32_t>(thinValues.size());

        // Handle record
        switch (static_cast<LLVMFunctionRecord>(record.id)) {
            default: {
                // Result value?
                if (HasValueAllocation(record.As<LLVMFunctionRecord>(), record.opCount)) {
                    ThinValue& value = thinValues.emplace_back();
                    value.kind = ThinValueKind::Instruction;
                    value.recordOffset = recordOffset;
                }

                // Add metadata and consume
                functionMd.instructionMetadata.emplace_back();

                // Always semantically relevant
                isSemanticInstruction = true;

                // Always in source
                recordOffset++;
                break;
            }

            case LLVMFunctionRecord::InstCall:
            case LLVMFunctionRecord::InstCall2: {
                uint32_t functionValueIndex = anchor - static_cast<uint32_t>(record.Op(3));
                
                ThinValue called = thinValues.at(functionValueIndex);
                ASSERT(called.kind == ThinValueKind::Function, "Mismatched thin type");
                
                // Ignore non-semantic instructions from cross-referencing
                if (called.bIsNonSemantic) {
                    ASSERT(thinTypes[called.thinType].function.isVoidReturn, "Unexpected function");
                    ParseDebugCall(functionMd, record, anchor, functionValueIndex);
                    isSemanticInstruction = false;
                } else {
                    functionMd.instructionMetadata.emplace_back();

                    // Always semantically relevant
                    isSemanticInstruction = true;
                }
                
                // Allocate return value if need be
                if (!thinTypes[called.thinType].function.isVoidReturn) {
                    ThinValue& value = thinValues.emplace_back();
                    value.kind = ThinValueKind::Instruction;
                    value.recordOffset = recordOffset;
                }

                // Increment source record on semantic
                if (!called.bIsNonSemantic) {
                    recordOffset++;
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

                if (isSemanticInstruction && functionMd.instructionMetadata.size()) {
                    functionMd.instructionMetadata.back() = metadata;
                }
                break;
            }

            case LLVMFunctionRecord::DebugLOCAgain: {
                // Repush pending
                if (isSemanticInstruction && functionMd.instructionMetadata.size()) {
                    functionMd.instructionMetadata.back() = metadata;
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
    // Value anchor
    uint32_t anchor = static_cast<uint32_t>(metadata.size());

    // Preallocate
    metadata.reserve(metadata.size() + block->records.size());

    // Visit records
    for (size_t i = 0; i < block->records.size(); i++) {
        const LLVMRecord &record = block->records[i];

        switch (static_cast<LLVMMetadataRecord>(record.id)) {
            default: {
                break;
            }

            case LLVMMetadataRecord::Kind: {
                // No value addition
                continue;
            }

            case LLVMMetadataRecord::Name: {
                // Set name
                LLVMRecordStringView recordName = LLVMRecordStringView(record, 0);

                // Validate next
                ASSERT(i + 1 != block->records.size(), "Expected succeeding metadata record");
                ASSERT(block->records[i + 1].Is(LLVMMetadataRecord::NamedNode), "Succeeding record to Name must be NamedNode");

                ParseNamedMetadata(block, anchor, block->records[++i], recordName);
                continue;
            }
        }

        // Setup md
        Metadata& md = metadata.emplace_back();
        md.type = static_cast<LLVMMetadataRecord>(record.id);
        md.record = &record;

        // Handle record
        switch (md.type) {
            default: {
                ASSERT(false, "Unhandled type");
                break;
            }

            case LLVMMetadataRecord::Node:
            case LLVMMetadataRecord::OldFnNode:
            case LLVMMetadataRecord::OldNode:
            case LLVMMetadataRecord::DistinctNode:
            case LLVMMetadataRecord::Location:
            case LLVMMetadataRecord::GenericDebug:
            case LLVMMetadataRecord::SubRange: 
            case LLVMMetadataRecord::Enumerator: 
            case LLVMMetadataRecord::BasicType: 
            case LLVMMetadataRecord::DerivedType: 
            case LLVMMetadataRecord::CompositeType: 
            case LLVMMetadataRecord::SubroutineType: 
            case LLVMMetadataRecord::Module: 
            case LLVMMetadataRecord::TemplateType: 
            case LLVMMetadataRecord::TemplateValue: 
            case LLVMMetadataRecord::GlobalVar: 
            case LLVMMetadataRecord::ObjProperty: 
            case LLVMMetadataRecord::ImportedEntity: 
            case LLVMMetadataRecord::StringOld: {
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

            case LLVMMetadataRecord::Value: {
                md.value = static_cast<uint32_t>(record.Op(1));
                break;
            }

            case LLVMMetadataRecord::LocalVar: {
                md.localVar.op = static_cast<LLVMDwarfOpKind>(record.Op(1));
                md.localVar.mdTypeId = static_cast<uint32_t>(record.Op(6));
                md.localVar.nameMdId = static_cast<uint32_t>(record.Op(3));
                break;
            }

            case LLVMMetadataRecord::Expression: {
                if (record.opCount > 1) {
                    md.expression.op = static_cast<LLVMDwarfOpKind>(record.Op(1));
                
                    switch (md.expression.op) {
                        default: {
                            break;
                        }
                        case LLVMDwarfOpKind::BitPiece: {
                            md.expression.bitPiece.bitStart = static_cast<uint32_t>(record.Op(2));
                            md.expression.bitPiece.bitEnd = static_cast<uint32_t>(record.Op(3));
                            break;
                        }
                    }
                }
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
        }
    }
}

void DXILDebugModule::ParseSymTab(LLVMBlock *block) {
    for (const LLVMRecord &record: block->records) {
        switch (static_cast<LLVMSymTabRecord>(record.id)) {
            default: {
                break;
            }
            case LLVMSymTabRecord::Entry: {
                /*
                 * LLVM Specification
                 *   VST_ENTRY: [valueid, namechar x N]
                 */

                // May not be mapped
                if (uint32_t value = record.Op32(0)) {
                    // Grow to capacity
                    if (valueStrings.size() <= value) {
                        valueStrings.resize(value + 1);
                        valueAllocations.resize(value + 1);
                    }

                    // Insert from operand 1
                    valueStrings[value] = LLVMRecordStringView(record, 1);

                    // Debugging experience
#ifndef NDEBUG
                    char buffer[256];
                    if (record.opCount < 256) {
                        record.FillOperands(buffer, 1);
                    }

                    GetValueAllocation(value);
#endif // NDEBUG
                }
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

const char* DXILDebugModule::GetValueAllocation(uint32_t id) {
    if (id >= valueStrings.size() || !valueStrings[id]) {
        return nullptr;
    }

    // Current view
    const LLVMRecordStringView& view = valueStrings[id];

    // Not allocated?
    if (!valueAllocations[id]) {
        valueAllocations[id] = blockAllocator.AllocateArray<char>(view.Length() + 1);
        view.CopyTerminated(valueAllocations[id]);
    }

    return valueAllocations[id];
}

const Backend::IL::Type* DXILDebugModule::GetTypeFromDwarf(uint32_t typeMdId) {
    Metadata& typeMd = metadata[typeMdId];
    return nullptr;
}

void DXILDebugModule::ParseDebugCall(FunctionMetadata& functionMd, const LLVMRecord &record, uint32_t anchor, uint32_t functionValueIndex) {
    // Determine call from string table
    if (functionValueIndex >= valueStrings.size()) {
        return;
    }
    
    LLVMRecordStringView view = valueStrings[functionValueIndex];

    // Debug value?
    if (view.StartsWith("llvm.dbg.value")) {
        ParseDebugValueCall(functionMd, record, anchor);
    }
}

void DXILDebugModule::ParseDebugValueCall(FunctionMetadata& functionMd, const LLVMRecord &record, uint32_t anchor) {
    // Interpret operands
    uint32_t valueMdIndex  = anchor - static_cast<uint32_t>(record.Op(4));
    uint32_t byteOffset    = anchor - static_cast<uint32_t>(record.Op(5));
    Metadata &variableMd   = this->metadata[anchor - static_cast<uint32_t>(record.Op(6))];
    Metadata& expressionMd = this->metadata[anchor - static_cast<uint32_t>(record.Op(7))];

    // No instruction to associate with?
    if (!functionMd.instructionMetadata.size()) {
        return;
    }

    // Optional, variable name
    char* variableName = nullptr;

    // Copy over name if possible
    if (variableMd.localVar.nameMdId) {
        LLVMRecordStringView name(*this->metadata[variableMd.localVar.nameMdId - 1].record, 0);
        variableName = blockAllocator.AllocateArray<char>(name.Length() + 1);
        name.CopyTerminated(variableName);
    }

    // By default, always associate with the last one
    InstructionDWARFInfo &set = functionMd.instructionDwarfInfos[static_cast<uint32_t>(functionMd.instructionMetadata.size() - 1)];
    set.name = variableName;
    set.typeMdId = variableMd.localVar.mdTypeId;

    // Setup value
    DXDwardValue& value = set.values.emplace_back();
    value.codeOffset = IL::InvalidID;
    value.kind = expressionMd.expression.op;

    // Get value reference
    if (Metadata &valueMd = this->metadata[valueMdIndex]; valueMd.type == LLVMMetadataRecord::Value) {
        // We cannot reliably cross-reference constants, just do records
        const ThinValue& debugValue = thinValues[valueMd.value];
        switch (debugValue.kind) {
            default:
                break;
            case ThinValueKind::Instruction:
                value.codeOffset = debugValue.recordOffset;
                break;
        }
    }

    // Copy over dwarf kind
    switch (value.kind) {
        default: {
            break;
        }
        case LLVMDwarfOpKind::BitPiece: {
            value.bitWise.bitStart = expressionMd.expression.bitPiece.bitStart;
            value.bitWise.bitEnd = expressionMd.expression.bitPiece.bitEnd;
            break;
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
