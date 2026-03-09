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
      unresolvedDwarfValues(allocators),
      valueStrings(allocators.Tag(kAllocModuleDXILSymbols)),
      valueAllocations(allocators.Tag(kAllocModuleDXILSymbols)),
      thinMetadata(allocators),
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

DXDwarfInfo DXILDebugModule::GetDwarfInfo(Backend::IL::TypeMap& typeMap, const IL::Function *function, uint32_t codeOffset) {
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

    // Copy over info
    DXDwarfInfo info;
    for (const InstructionDwarfVariable& sourceVar : it->second.variables) {
        DXDwarfVariableValue& variable = info.variables.emplace_back();
        variable.name = sourceVar.name;
        variable.type = GetTypeFromDwarf(typeMap, sourceVar.typeMdId - 1);
        variable.variableId = sourceVar.variableMdId;

        for (const InstructionDwarfValue* sourceValue : sourceVar.values) {
            DXDwarfValue &value = variable.values.emplace_back();
            value.kind = sourceValue->kind;
            value.code = sourceValue->code;
            value.bitWise.bitStart = sourceValue->bitWise.bitStart;
            value.bitWise.bitLength = sourceValue->bitWise.bitLength;
        } 
    }
    
    return info;
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
            case LLVMTypeRecord::Integer: {
                type.integral.bitWidth = record.Op32(0);
                break;
            }
            case LLVMTypeRecord::Half: {
                type.integral.bitWidth = 16;
                break;
            }
            case LLVMTypeRecord::Float: {
                type.integral.bitWidth = 32;
                break;
            }
            case LLVMTypeRecord::Double: {
                type.integral.bitWidth = 64;
                break;
            }
            case LLVMTypeRecord::Vector: {
                type.aggregate.contained = record.Op32(1);
                break;
            }
            case LLVMTypeRecord::Array: {
                type.aggregate.contained = record.Op32(1);
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
    // Keep current heads
    const size_t valueHead = thinValues.size();
    const size_t metadataHead = thinMetadata.size();

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
    InstructionMetadata instrMetadata;

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
                instrMetadata.sourceAssociation.fileUID = 0;
                instrMetadata.sourceAssociation.line = record.OpAs<uint32_t>(0) - 1;
                instrMetadata.sourceAssociation.column = record.OpAs<uint32_t>(1) - 1;

                // Has scope?
                if (uint32_t scope = record.OpAs<uint32_t>(2); scope) {
                    instrMetadata.sourceAssociation.fileUID = static_cast<uint16_t>(GetLinearFileUID(scope - 1));
                }

                if (isSemanticInstruction && functionMd.instructionMetadata.size()) {
                    functionMd.instructionMetadata.back() = instrMetadata;
                }
                break;
            }

            case LLVMFunctionRecord::DebugLOCAgain: {
                // Repush pending
                if (isSemanticInstruction && functionMd.instructionMetadata.size()) {
                    functionMd.instructionMetadata.back() = instrMetadata;
                }
                break;
            }
        }
    }

    // Resolve all pending values
    for (InstructionDwarfValue *value : unresolvedDwarfValues) {
        ResolveDwarfValue(value);
    }

    // Cleanup
    unresolvedDwarfValues.clear();

    // Reset heads, value indices reset after function blocks
    thinValues.resize(valueHead);
    thinMetadata.resize(metadataHead);
}

void DXILDebugModule::ParseConstants(LLVMBlock *block) {
    uint32_t type = IL::InvalidID;
    
    for (LLVMRecord &record: block->records) {
        if (record.Is(LLVMConstantRecord::SetType)) {
            type = record.Op32(0);
            continue;
        }

        // Constants are resolved on demand
        ThinValue &value = thinValues.emplace_back();
        value.kind = ThinValueKind::Constant;
        value.thinType = type;
        value.record = &record;
    }
}

void DXILDebugModule::ParseMetadata(LLVMBlock *block) {
    // Value anchor
    uint32_t anchor = static_cast<uint32_t>(thinMetadata.size());

    // Preallocate
    thinMetadata.reserve(thinMetadata.size() + block->records.size());

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
        Metadata& md = thinMetadata.emplace_back();
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
            case LLVMMetadataRecord::Enumerator: 
            case LLVMMetadataRecord::SubroutineType: 
            case LLVMMetadataRecord::Module: 
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
                            md.expression.bitPiece.bitLength = static_cast<uint32_t>(record.Op(3));
                            break;
                        }
                    }
                }
                break;
            }

            case LLVMMetadataRecord::DerivedType: {
                if (record.opCount > 1) {
                    md.derivedType.tag = static_cast<LLVMDwarfTag>(record.Op(1));

                    switch (md.derivedType.tag) {
                        default: {
                            break;
                        }
                        case LLVMDwarfTag::Typedef: {
                            md.derivedType._typedef.nameMdId = static_cast<uint32_t>(record.Op(2));
                            md.derivedType._typedef.baseTypeMdId = static_cast<uint32_t>(record.Op(6));
                            break;
                        }
                        case LLVMDwarfTag::Member: {
                            md.derivedType.member.nameMdId = static_cast<uint32_t>(record.Op(2));
                            md.derivedType.member.baseTypeMdId = static_cast<uint32_t>(record.Op(6));
                            md.derivedType.member.size = static_cast<uint32_t>(record.Op(7));
                            md.derivedType.member.align = static_cast<uint32_t>(record.Op(8));
                            md.derivedType.member.offset = static_cast<uint32_t>(record.Op(9));
                            break;
                        }
                        case LLVMDwarfTag::Const: {
                            md.derivedType._const.baseTypeMdId = static_cast<uint32_t>(record.Op(6));
                            break;
                        }
                    }
                }
                break;
            }

            case LLVMMetadataRecord::CompositeType: {
                if (record.opCount > 1) {
                    md.compositeType.tag = static_cast<LLVMDwarfTag>(record.Op(1));

                    switch (md.compositeType.tag) {
                        default: {
                            break;
                        }
                        case LLVMDwarfTag::ClassType: {
                            md.compositeType._class.nameMdId = static_cast<uint32_t>(record.Op(2));
                            md.compositeType._class.size = static_cast<uint32_t>(record.Op(7));
                            md.compositeType._class.align = static_cast<uint32_t>(record.Op(8));
                            md.compositeType._class.elementsMdId = static_cast<uint32_t>(record.Op(11));
                            md.compositeType._class.templateParamsMdId = static_cast<uint32_t>(record.Op(14));
                            break;
                        }
                        case LLVMDwarfTag::StructureType: {
                            md.compositeType.structureType.nameMdId = static_cast<uint32_t>(record.Op(2));
                            md.compositeType.structureType.size = static_cast<uint32_t>(record.Op(7));
                            md.compositeType.structureType.align = static_cast<uint32_t>(record.Op(8));
                            md.compositeType.structureType.elementsMdId = static_cast<uint32_t>(record.Op(11));
                            break;
                        }
                        case LLVMDwarfTag::Array: {
                            md.compositeType.arrayType.baseTypeMdId = static_cast<uint32_t>(record.Op(6));
                            md.compositeType.arrayType.size = static_cast<uint32_t>(record.Op(7));
                            md.compositeType.arrayType.align = static_cast<uint32_t>(record.Op(8));
                            md.compositeType.arrayType.elementsMdId = static_cast<uint32_t>(record.Op(11));
                            break;
                        }
                    }
                }
                break;
            }
                
            case LLVMMetadataRecord::SubRange: {
                md.subRange.count = static_cast<uint32_t>(record.Op(1));
                md.subRange.lowerBound = static_cast<uint32_t>(record.Op(2));
                break;
            }

            case LLVMMetadataRecord::TemplateType: {
                md.templateType.nameMdId = static_cast<uint32_t>(record.Op(1));
                md.templateType.typeMdId = static_cast<uint32_t>(record.Op(2));
                break;
            }

            case LLVMMetadataRecord::TemplateValue: {
                md.templateValue.nameMdId = static_cast<uint32_t>(record.Op(2));
                md.templateValue.typeMdId = static_cast<uint32_t>(record.Op(3));
                md.templateValue.valueMdId = static_cast<uint32_t>(record.Op(4));
                break;
            }

            case LLVMMetadataRecord::BasicType: {
                md.basicType.nameMdId = static_cast<uint32_t>(record.Op(2));
                md.basicType.size = static_cast<uint32_t>(record.Op(3));
                md.basicType.align = static_cast<uint32_t>(record.Op(4));
                md.basicType.encoding = static_cast<LLVMDwarfTypeEncoding>(record.Op(5));
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
                ParseContentsRecord(block, static_cast<uint32_t>(record.Op(i)));
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

template<typename T>
void DXILDebugModule::ParseContentsAdapter(const T &filename, const T &contents) {
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

void DXILDebugModule::ParseContentsRecord(LLVMBlock* block, uint32_t fileMdId) {
    const LLVMRecord& record = block->records[fileMdId];

    // Get strings
    LLVMRecordStringView filename(block->records[record.Op(0) - 1], 0);
    LLVMRecordStringView contents(block->records[record.Op(1) - 1], 0);
    ParseContentsAdapter(filename, contents);
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
    Metadata& md = thinMetadata[scopeMdId];

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
    Metadata& fileMd = thinMetadata[fileMdId - 1];
    ASSERT(fileMd.type == LLVMMetadataRecord::File, "Unexpected node");

    // OK
    return fileMd.file.linearFileUID;
}

struct StringViewAdapter {
    bool StartsWithOffset(uint32_t offset, std::string_view str) const {
        if (str.length() > view.length() - offset) {
            return false;
        }

        for (uint32_t i = 0; i < str.length(); i++) {
            if (str[i] != static_cast<char>(view[offset + i])) {
                return false;
            }
        }

        return true;
    }

    void SubStr(uint64_t begin, uint64_t end, char* buffer) const {
        for (size_t i = begin; i < end; i++) {
            buffer[i - begin] = static_cast<char>(view[i]);
        }
    }
    
    void SubStrTerminated(uint64_t begin, uint64_t end, char* buffer) const {
        for (size_t i = begin; i < end; i++) {
            buffer[i - begin] = static_cast<char>(view[i]);
        }

        // Terminator
        buffer[end - begin] = '\0';
    }
            
    template<typename F>
    void CopyUntilTerminated(uint64_t begin, char* buffer, uint32_t length, F&& functor) const {
        size_t i;
        for (i = begin; i < std::min<size_t>(begin + length - 1, view.length()); i++) {
            char ch = view[i];

            // Break?
            if (!functor(ch)) {
                break;
            }

            // Append
            buffer[i - begin] = ch;
        }

        // Terminator
        buffer[i - begin] = '\0';
    }

    uint32_t Length() const {
        return static_cast<uint32_t>(view.length());
    }

    char operator[](uint32_t i) const {
        return view[i];
    }

    operator std::string_view() const {
        return view;
    }
    
    std::string_view view;
};

void DXILDebugModule::CreateFragmentsFromSourceBlock() {
    // A single file either indicates that there's a single file, or, that the contents are unresolved
    // f.x. line directives that need to be mapped
    isContentsUnresolved = (shaderSourceInfo.sourceFiles.size() == 1);

    // Fill all files
    for (const DXBCPhysicalBlockShaderSourceInfo::SourceFile& file : shaderSourceInfo.sourceFiles) {
        ParseContentsAdapter(
            StringViewAdapter{ file.filename },
            StringViewAdapter{ file.contents }
        );
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

const Backend::IL::Type* DXILDebugModule::GetTypeFromDwarf(Backend::IL::TypeMap& typeMap, uint32_t typeMdId) {
    Metadata& typeMd = thinMetadata[typeMdId];

    switch (typeMd.type) {
        default: {
            ASSERT(false, "Unexpected type");
            break;
        }
        case LLVMMetadataRecord::DerivedType: {
            switch (typeMd.derivedType.tag) {
                default: {
                    ASSERT(false, "Unexpected derived tag");
                    break;
                }
                case LLVMDwarfTag::Typedef: {
                    return GetTypeFromDwarf(typeMap, typeMd.derivedType._typedef.baseTypeMdId - 1);
                }
                case LLVMDwarfTag::Const: {
                    return GetTypeFromDwarf(typeMap, typeMd.derivedType._const.baseTypeMdId - 1);
                }
            }
            break;
        }
        case LLVMMetadataRecord::CompositeType: {
            switch (typeMd.compositeType.tag) {
                default: {
                    ASSERT(false, "Unexpected composite tag");
                    break;
                }
                case LLVMDwarfTag::ClassType: {
                    return GetClassTypeFromDwarf(typeMap, typeMd);
                }
                case LLVMDwarfTag::StructureType: {
                    return GetStructureTypeFromDwarf(typeMap, typeMd);
                }
                case LLVMDwarfTag::Array: {
                    return GetArrayTypeFromDwarf(typeMap, typeMd);
                }
            }
            break;
        }
        case LLVMMetadataRecord::BasicType: {
            return GetBasicTypeFromDwarf(typeMap, typeMd);
        }
    }
    
    return nullptr;
}

const Backend::IL::Type * DXILDebugModule::GetClassTypeFromDwarf(Backend::IL::TypeMap &typeMap, const Metadata &typeMd) {
    Metadata& memberListMd = thinMetadata[typeMd.compositeType._class.elementsMdId - 1];

    // All elements
    TrivialStackVector<const Backend::IL::Type*, 16> elements;

    // Current bit offset
    uint32_t bitOffset = 0;

    // Populate all elements
    for (uint32_t i = 0; i < memberListMd.record->opCount; i++) {
        Metadata& memberMd = thinMetadata[memberListMd.record->Op32(i) - 1];
        ASSERT(memberMd.type == LLVMMetadataRecord::DerivedType, "Unexpected type");
        ASSERT(memberMd.derivedType.tag == LLVMDwarfTag::Member, "Unexpected tag");

        // Aligned?
        if (memberMd.derivedType.member.offset != bitOffset) {
            ASSERT(memberMd.derivedType.member.offset > bitOffset, "Out of order declaration");

            // Get padding requirement
            uint32_t bitPadding = memberMd.derivedType.member.offset - bitOffset;
            ASSERT(bitPadding % 8 == 0, "Padding not byte aligned");

            // Insert dummy padding
            elements.Add(typeMap.FindTypeOrAdd(Backend::IL::ArrayType {
                .elementType = typeMap.FindTypeOrAdd(Backend::IL::IntType { .bitWidth = 8, .signedness = false }),
                .count = bitPadding / 8
            }));
        }

        // Get member type
        elements.Add(GetTypeFromDwarf(typeMap, memberMd.derivedType.member.baseTypeMdId - 1));
        
        // Next offset
        bitOffset = memberMd.derivedType.member.offset + memberMd.derivedType.member.size;
    }

    // Name of the composite
    LLVMRecordStringView name(*thinMetadata[typeMd.compositeType._class.nameMdId - 1].record, 0);

    // If vector, try to represent it as such
    if (name.StartsWith("vector")) {
        ASSERT(elements.Size() <= 4, "Unexpected vector length");

        // Vector elements must all match
        bool bAllMatching = true;
        for (uint64_t i = 1; i < elements.Size(); i++) {
            bAllMatching &= elements[i] == elements[0];
        }

        if (bAllMatching) {
            return typeMap.FindTypeOrAdd(Backend::IL::VectorType {
                .containedType = elements[0],
                .dimension = static_cast<uint8_t>(elements.Size())
            });
        }
    }

    // If matrix, try to represent it as such
    if (name.StartsWith("matrix")) {
        ASSERT(elements.Size() <= 16, "Unexpected matrix length");

        // Attributes
        uint32_t rows    = 0;
        uint32_t columns = 0;
        
        Metadata& templateListMd = thinMetadata[typeMd.compositeType._class.templateParamsMdId - 1];

        // Parse all arguments
        for (uint32_t i = 0; i < templateListMd.record->opCount; i++) {
            Metadata& argMd = thinMetadata[templateListMd.record->Op32(i) - 1];
            switch (argMd.type) {
                default: {
                    break;
                }
                case LLVMMetadataRecord::TemplateType: {
                    break;
                }
                case LLVMMetadataRecord::TemplateValue: {
                    LLVMRecordStringView argName(*thinMetadata[argMd.templateValue.nameMdId - 1].record, 0);

                    if (argName.StartsWith("row_count")) {
                        Metadata& valueMd = thinMetadata[argMd.templateValue.valueMdId - 1];
                        ASSERT(valueMd.type == LLVMMetadataRecord::Value, "Unexpected record");
                        rows = static_cast<uint32_t>(GetLiteralConstant(valueMd.value));
                    } else if (argName.StartsWith("col_count")) {
                        Metadata& valueMd = thinMetadata[argMd.templateValue.valueMdId - 1];
                        ASSERT(valueMd.type == LLVMMetadataRecord::Value, "Unexpected record");
                        columns = static_cast<uint32_t>(GetLiteralConstant(valueMd.value));
                    }
                    break;
                }
            }
        }
        
        // Matrix elements must all match
        bool bAllMatching = true;
        for (uint64_t i = 1; i < elements.Size(); i++) {
            bAllMatching &= elements[i] == elements[0];
        }

        if (bAllMatching) {
            return typeMap.FindTypeOrAdd(Backend::IL::MatrixType {
                .containedType = elements[0],
                .rows = static_cast<uint8_t>(rows),
                .columns = static_cast<uint8_t>(columns)
            });
        }
    }

    // Otherwise assume struct
    Backend::IL::StructType _struct;
    for (const Backend::IL::Type* element : elements) {
        _struct.memberTypes.push_back(element);
    }
    
    return typeMap.FindTypeOrAdd(_struct);
}

const Backend::IL::Type * DXILDebugModule::GetStructureTypeFromDwarf(Backend::IL::TypeMap &typeMap, const Metadata &typeMd) {
    Metadata& memberListMd = thinMetadata[typeMd.compositeType.structureType.elementsMdId - 1];

    // All elements
    TrivialStackVector<const Backend::IL::Type*, 16> elements;

    // Current bit offset
    uint32_t bitOffset = 0;

    // Populate all elements
    for (uint32_t i = 0; i < memberListMd.record->opCount; i++) {
        Metadata& memberMd = thinMetadata[memberListMd.record->Op32(i) - 1];
        ASSERT(memberMd.type == LLVMMetadataRecord::DerivedType, "Unexpected type");
        ASSERT(memberMd.derivedType.tag == LLVMDwarfTag::Member, "Unexpected tag");

        if (memberMd.derivedType.member.offset != bitOffset) {
            ASSERT(memberMd.derivedType.member.offset > bitOffset, "Out of order declaration");

            // Get padding requirement
            uint32_t bitPadding = memberMd.derivedType.member.offset - bitOffset;
            ASSERT(bitPadding % 8 == 0, "Padding not byte aligned");

            // Insert dummy padding
            elements.Add(typeMap.FindTypeOrAdd(Backend::IL::ArrayType {
                .elementType = typeMap.FindTypeOrAdd(Backend::IL::IntType { .bitWidth = 8, .signedness = false }),
                .count = bitPadding / 8
            }));
        }

        // Get member type
        elements.Add(GetTypeFromDwarf(typeMap, memberMd.derivedType.member.baseTypeMdId - 1));

        // Next offset
        bitOffset = memberMd.derivedType.member.offset + memberMd.derivedType.member.size;
    }

    // Otherwise assume struct
    Backend::IL::StructType _struct;
    for (const Backend::IL::Type* element : elements) {
        _struct.memberTypes.push_back(element);
    }
    
    return typeMap.FindTypeOrAdd(_struct);
}

const Backend::IL::Type * DXILDebugModule::GetArrayTypeFromDwarf(Backend::IL::TypeMap &typeMap, const Metadata &typeMd) {
    Metadata& elementsListMd = thinMetadata[typeMd.compositeType.arrayType.elementsMdId - 1];

    // Base type we start from
    const Backend::IL::Type* baseType = GetTypeFromDwarf(typeMap, typeMd.compositeType.arrayType.baseTypeMdId - 1);
    
    // Populate all sub-ranges
    for (uint32_t i = 0; i < elementsListMd.record->opCount; i++) {
        Metadata& subRangeMd = thinMetadata[elementsListMd.record->Op32(i) - 1];
        ASSERT(subRangeMd.subRange.lowerBound == 0, "Lower bounds not supported");
        
        baseType = typeMap.FindTypeOrAdd(Backend::IL::ArrayType {
            .elementType = baseType,
            .count = subRangeMd.subRange.count
        });
    }
    
    return baseType;
}

const Backend::IL::Type * DXILDebugModule::GetBasicTypeFromDwarf(Backend::IL::TypeMap &typeMap, const Metadata &typeMd) {
    switch (typeMd.basicType.encoding) {
        default: {
            ASSERT(false, "Unexpected composite tag");
            break;
        }
        case LLVMDwarfTypeEncoding::Bool: {
            return typeMap.FindTypeOrAdd(Backend::IL::BoolType {});
        }
        case LLVMDwarfTypeEncoding::Float: {
            return typeMap.FindTypeOrAdd(Backend::IL::FPType {
                .bitWidth = static_cast<uint8_t>(typeMd.basicType.size)
            });
        }
        case LLVMDwarfTypeEncoding::Signed: {
            return typeMap.FindTypeOrAdd(Backend::IL::IntType {
                .bitWidth = static_cast<uint8_t>(typeMd.basicType.size),
                .signedness = true
            });
        }
        case LLVMDwarfTypeEncoding::Unsigned: {
            return typeMap.FindTypeOrAdd(Backend::IL::IntType {
                .bitWidth = static_cast<uint8_t>(typeMd.basicType.size),
                .signedness = false
            });
        }
    }
    
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
    uint32_t valueMdIndex    = anchor - static_cast<uint32_t>(record.Op(4));
    uint32_t byteOffset      = anchor - static_cast<uint32_t>(record.Op(5));
    uint32_t variableMdIndex = anchor - static_cast<uint32_t>(record.Op(6));
    Metadata &variableMd     = thinMetadata[variableMdIndex];
    Metadata& expressionMd   = thinMetadata[anchor - static_cast<uint32_t>(record.Op(7))];

    // No instruction to associate with?
    if (!functionMd.instructionMetadata.size()) {
        return;
    }

    // By default, always associate with the last one
    InstructionDwarfInfo &set = functionMd.instructionDwarfInfos[static_cast<uint32_t>(functionMd.instructionMetadata.size() - 1)];

    // Try to find existing variable
    InstructionDwarfVariable* variable = nullptr;
    for (InstructionDwarfVariable& candidate : set.variables) {
        if (candidate.variableMdId == variableMdIndex) {
            variable = &candidate;
            break;
        }
    }

    // None found, create a new one
    if (!variable) {
        // Optional, variable name
        char* variableName = nullptr;

        // Copy over name if possible
        if (variableMd.localVar.nameMdId) {
            LLVMRecordStringView name(*thinMetadata[variableMd.localVar.nameMdId - 1].record, 0);
            variableName = blockAllocator.AllocateArray<char>(name.Length() + 1);
            name.CopyTerminated(variableName);
        }
        
        variable = &set.variables.emplace_back();
        variable->name = variableName;
        variable->variableMdId = variableMdIndex;
        variable->typeMdId = variableMd.localVar.mdTypeId;
    }
    
    // Setup value
    InstructionDwarfValue* value = variable->values.emplace_back(blockAllocator.Allocate<InstructionDwarfValue>());
    value->valueId = IL::InvalidID;
    value->kind = expressionMd.expression.op;
    
    if (Metadata &valueMd = thinMetadata[valueMdIndex]; valueMd.type == LLVMMetadataRecord::Value) {
        value->valueId = valueMd.value;

        // May not be resolved
        if (value->valueId < thinValues.size()) {
            ResolveDwarfValue(value);
        } else {
            unresolvedDwarfValues.push_back(value);
        }
    }
    
    // Copy over dwarf kind
    switch (value->kind) {
        default: {
            break;
        }
        case LLVMDwarfOpKind::BitPiece: {
            value->bitWise.bitStart = expressionMd.expression.bitPiece.bitStart;
            value->bitWise.bitLength = expressionMd.expression.bitPiece.bitLength;
            break;
        }
    }
}

void DXILDebugModule::ResolveDwarfValue(InstructionDwarfValue* value) {
    // We cannot reliably cross-reference constants, just do records
    const ThinValue& debugValue = thinValues[value->valueId];
    switch (debugValue.kind) {
        default: {
            ASSERT(false, "Unexpected value");
            return;
        }
        case ThinValueKind::Instruction: {
            value->code.codeOffset = debugValue.recordOffset;
            break;
        }
        case ThinValueKind::Constant: {
            value->code.constant = ResolveConstant(value->valueId);
            break;
        }
    }
}

template<typename T>
const T* DXILDebugModule::AllocateThinConstant(const T& decl) {
    T* value = blockAllocator.Allocate<T>(decl);
    value->kind = T::kKind;
    return value;
}

const IL::Constant* DXILDebugModule::ResolveConstant(uint32_t valueId) {
    const ThinValue& debugValue = thinValues[valueId];
    
    // Type assigned
    const ThinType& thinType = thinTypes[debugValue.thinType];
    
    // Resolve as much as possible
    switch (static_cast<LLVMConstantRecord>(debugValue.record->id)) {
        default: {
            return AllocateThinConstant(IL::UnexposedConstant {});
        }
            
        case LLVMConstantRecord::Null: {
            return AllocateThinConstant(IL::NullConstant {});
        }

        case LLVMConstantRecord::Integer: {
            if (thinType.integral.bitWidth == 1) {
                return AllocateThinConstant(IL::BoolConstant {
                    .value = debugValue.record->Op32(0) ? true : false
                });
            } else {
                return AllocateThinConstant(IL::IntConstant {
                    .value = debugValue.record->Op32(0)
                });
            }
        }

        case LLVMConstantRecord::Float: {
            return AllocateThinConstant(IL::FPConstant {
                .value = debugValue.record->OpBitCast<float>(0)
            });
        }

        case LLVMConstantRecord::Aggregate: {
            switch (thinType.type) {
                default: {
                    return AllocateThinConstant(IL::UnexposedConstant {});
                }
                case LLVMTypeRecord::StructAnon:
                case LLVMTypeRecord::StructName:
                case LLVMTypeRecord::StructNamed: {
                    IL::StructConstant decl;

                    // Fill members
                    for (uint32_t i = 0; i < debugValue.record->opCount; i++) {
                        uint32_t operand = debugValue.record->Op32(i);
                        
                        if (operand < thinValues.size()) {
                            const IL::Constant *memberConstant = ResolveConstant(operand);
                            decl.members.push_back(memberConstant);
                        } else {
                            return AllocateThinConstant(IL::UnexposedConstant {});
                        }
                    }

                    return AllocateThinConstant(decl);
                }
                case LLVMTypeRecord::Vector: {
                    IL::VectorConstant decl;

                    // Fill members
                    for (uint32_t i = 0; i < debugValue.record->opCount; i++) {
                        uint32_t operand = debugValue.record->Op32(i);
                        
                        if (operand < thinValues.size()) {
                            const IL::Constant *memberConstant = ResolveConstant(operand);
                            decl.elements.push_back(memberConstant);
                        } else {
                            return AllocateThinConstant(IL::UnexposedConstant {});
                        }
                    }

                    return AllocateThinConstant(decl);
                }
                case LLVMTypeRecord::Array: {
                    IL::ArrayConstant decl;

                    // Fill members
                    for (uint32_t i = 0; i < debugValue.record->opCount; i++) {
                        uint32_t operand = debugValue.record->Op32(i);
                        
                        if (operand < thinValues.size()) {
                            const IL::Constant *memberConstant = ResolveConstant(operand);
                            decl.elements.push_back(memberConstant);
                        } else {
                            return AllocateThinConstant(IL::UnexposedConstant {});
                        }
                    }

                    return AllocateThinConstant(decl);
                }
            }
        }

        case LLVMConstantRecord::Data: {
            switch (thinType.type) {
                default: {
                    return AllocateThinConstant(IL::UnexposedConstant {});
                }
                case LLVMTypeRecord::Vector: {
                    IL::VectorConstant decl;

                    const ThinType& containedThinType = thinTypes[thinType.aggregate.contained];
                    
                    // Fill members
                    for (uint32_t i = 0; i < debugValue.record->opCount; i++) {
                        switch (containedThinType.type) {
                            default: {
                                ASSERT(false, "Invalid type");
                                decl.elements.push_back(AllocateThinConstant(IL::UnexposedConstant {}));
                            }
                            case LLVMTypeRecord::Integer: {
                                if (thinType.integral.bitWidth == 1) {
                                    decl.elements.push_back(AllocateThinConstant(IL::BoolConstant {
                                        .value = debugValue.record->Op32(0) ? true : false
                                    }));
                                } else {
                                    decl.elements.push_back(AllocateThinConstant(IL::IntConstant {
                                        .value = debugValue.record->Op32(0)
                                    }));
                                }
                                break;
                            }
                            case LLVMTypeRecord::Half:
                            case LLVMTypeRecord::Float:
                            case LLVMTypeRecord::Double: {
                                decl.elements.push_back(AllocateThinConstant(IL::FPConstant {
                                    .value = debugValue.record->OpBitCast<float>(0)
                                }));
                            }
                        }
                    }

                    return AllocateThinConstant(decl);
                }
                case LLVMTypeRecord::Array: {
                    IL::ArrayConstant decl;

                    const ThinType& containedThinType = thinTypes[thinType.aggregate.contained];
                    
                    // Fill members
                    for (uint32_t i = 0; i < debugValue.record->opCount; i++) {
                        switch (containedThinType.type) {
                            default: {
                                ASSERT(false, "Invalid type");
                                decl.elements.push_back(AllocateThinConstant(IL::UnexposedConstant {}));
                            }
                            case LLVMTypeRecord::Integer: {
                                if (thinType.integral.bitWidth == 1) {
                                    decl.elements.push_back(AllocateThinConstant(IL::BoolConstant {
                                        .value = debugValue.record->Op32(0) ? true : false
                                    }));
                                } else {
                                    decl.elements.push_back(AllocateThinConstant(IL::IntConstant {
                                        .value = debugValue.record->Op32(0)
                                    }));
                                }
                                break;
                            }
                            case LLVMTypeRecord::Half:
                            case LLVMTypeRecord::Float:
                            case LLVMTypeRecord::Double: {
                                decl.elements.push_back(AllocateThinConstant(IL::FPConstant {
                                    .value = debugValue.record->OpBitCast<float>(0)
                                }));
                            }
                        }
                    }

                    return AllocateThinConstant(decl);
                }
            }
        }
    }
}

uint64_t DXILDebugModule::GetLiteralConstant(uint32_t valueId) {
    return LLVMBitStreamReader::DecodeSigned(thinValues[valueId].record->Op(0));
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
