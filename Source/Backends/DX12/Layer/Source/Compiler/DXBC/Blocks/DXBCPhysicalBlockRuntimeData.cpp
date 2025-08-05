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

#include <Backends/DX12/Compiler/DXBC/Blocks/DXBCPhysicalBlockRuntimeData.h>
#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXBC/DXBCParseContext.h>
#include <Backends/DX12/Compiler/DXIL/DXILModule.h>
#include <Backends/DX12/Compiler/DXCompileJob.h>

// Common
#include <Common/Format.h>

// Std
#include <set>

/// DXC emits resource declarations in a particular order, mirror it for validation purposes
static DXILShaderResourceClass resourceEmitOrder[] = {
    DXILShaderResourceClass::CBVs,
    DXILShaderResourceClass::Samplers,
    DXILShaderResourceClass::SRVs,
    DXILShaderResourceClass::UAVs
};

DXBCPhysicalBlockRuntimeData::DXBCPhysicalBlockRuntimeData(const Allocators &allocators, Backend::IL::Program &program, DXBCPhysicalBlockTable &table)
    : DXBCPhysicalBlockSection(allocators, program, table),
      indexBuffer(allocators),
      rawBuffer(allocators),
      stringBuffer(allocators),
      parts(allocators) {
    /* */
}

void DXBCPhysicalBlockRuntimeData::Parse() {
    // Block is optional
    DXBCPhysicalBlock *block = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::RuntimeData);
    if (!block) {
        return;
    }

    // Setup parser
    DXBCParseContext ctx(block->ptr, block->length);

    // Get header
    header = ctx.Consume<DXBCRuntimeDataHeader>();

    // Determine part offsets
    TrivialStackVector<uint32_t, 8u> partOffsets;
    for (uint32_t i = 0; i < header.partCount; i++) {
        partOffsets.Add(ctx.Consume<uint32_t>());
    }

    // Parse each part
    for (uint32_t i = 0; i < header.partCount; i++) {
        ctx.SetOffset(partOffsets[i]);

        // Get part header
        auto partHeader = ctx.Consume<DXBCRuntimeDataPartHeader>();

        // Create part
        Part& part = parts.emplace_back(allocators);
        part.type = partHeader.type;
        part.ptr = ctx.ptr;
        part.length = partHeader.size;

        // Parse the part contents
        DXBCParseContext partCtx(ctx.ptr, partHeader.size);
        switch (partHeader.type) {
            default: {
                ASSERT(false, "Unsupported RDAT part");
                break;
            }
            case DXBCRuntimeDataPartType::String:
                ParseStringPart(partCtx);
                break;
            case DXBCRuntimeDataPartType::IndexArray:
                ParseIndexArraysPart(partCtx);
                break;
            case DXBCRuntimeDataPartType::ResourceTable:
                ParseTablePart(partCtx, resourceRecords);
                break;
            case DXBCRuntimeDataPartType::FunctionTable:
                ParseTablePart(partCtx, functionRecords);
                break;
            case DXBCRuntimeDataPartType::RawBytes:
                ParseRawBytesPart(partCtx);
                break;
            case DXBCRuntimeDataPartType::SubObjectTable:
                ParseTablePart(partCtx, subObjectRecords);
                break;
            case DXBCRuntimeDataPartType::NodeIDTable:
                ParseTablePart(partCtx, nodeIDRecords);
                break;
            case DXBCRuntimeDataPartType::NodeShaderIOAttribTable:
                ParseTablePart(partCtx, nodeShaderIOAttribRecords);
                break;
            case DXBCRuntimeDataPartType::NodeShaderFuncAttribTable:
                ParseTablePart(partCtx, nodeShaderFuncAttribRecords);
                break;
            case DXBCRuntimeDataPartType::IONodeTable:
                ParseTablePart(partCtx, ioNodeRecords);
                break;
            case DXBCRuntimeDataPartType::NodeShaderInfoTable:
                ParseTablePart(partCtx, nodeShaderInfoRecords);
                break;
            case DXBCRuntimeDataPartType::MeshNodesPreviewInfoTable:
                ASSERT(false, "Experimental RDAT part not supported");
                break;
            case DXBCRuntimeDataPartType::SignatureElementTable:
                ParseTablePart(partCtx, signatureElementRecords);
                break;
            case DXBCRuntimeDataPartType::VSInfoTable:
                ParseTablePart(partCtx, vsInfoRecords);
                break;
            case DXBCRuntimeDataPartType::PSInfoTable:
                ParseTablePart(partCtx, psInfoRecords);
                break;
            case DXBCRuntimeDataPartType::HSInfoTable:
                ParseTablePart(partCtx, hsInfoRecords);
                break;
            case DXBCRuntimeDataPartType::DSInfoTable:
                ParseTablePart(partCtx, dsInfoRecords);
                break;
            case DXBCRuntimeDataPartType::GSInfoTable:
                ParseTablePart(partCtx, gsInfoRecords);
                break;
            case DXBCRuntimeDataPartType::CSInfoTable:
                ParseTablePart(partCtx, csInfoRecords);
                break;
            case DXBCRuntimeDataPartType::MSInfoTable:
                ParseTablePart(partCtx, msInfoRecords);
                break;
            case DXBCRuntimeDataPartType::ASInfoTable:
                ParseTablePart(partCtx, asInfoRecords);
                break;
        }
    }

    // Finalize resources
    for (uint64_t i = 0; i < resourceRecords.records.size(); i++) {
        ResourceEntry& entry = resourceRecords.records[i];
        entry.name = GetString(entry.record.nameOffset);
    }

    // Finalize functions
    for (FunctionEntry& entry : functionRecords.records) {
        entry.name = GetString(entry.record.nameOffset);
        entry.unmangledName = GetString(entry.record.unmangledNameOffset);

        // Setup resources
        if (entry.record.resourceRecordIndex != UINT32_MAX) {
            entry.resources.resize(indexBuffer[entry.record.resourceRecordIndex]);
        }

        // Setup dependencies
        if (entry.record.dependenciesStringIndex != UINT32_MAX) {
            entry.dependencies.resize(indexBuffer[entry.record.dependenciesStringIndex]);
        }

        // Copy over actual resource indices
        for (uint64_t i = 0; i < entry.resources.size(); i++) {
            entry.resources[i] = indexBuffer[entry.record.resourceRecordIndex + i + 1];
        }

        // Copy over actual function indices
        for (uint64_t i = 0; i < entry.dependencies.size(); i++) {
            entry.resources[i] = indexBuffer[entry.record.dependenciesStringIndex + i + 1];
        }
    }

    // Finalize subobjects
    for (uint64_t i = 0; i < subObjectRecords.records.size(); i++) {
        SubObjectEntry& entry = subObjectRecords.records[i];
        entry.name = GetString(entry.record.nameOffset);
    }
}

void DXBCPhysicalBlockRuntimeData::ParseIndexArraysPart(DXBCParseContext& partCtx) {
    indexBuffer.resize(partCtx.PendingBytes() / sizeof(uint32_t));
    std::memcpy(indexBuffer.data(), partCtx.ptr, partCtx.PendingBytes());
}

void DXBCPhysicalBlockRuntimeData::ParseRawBytesPart(DXBCParseContext &partCtx) {
    rawBuffer.resize(partCtx.PendingBytes());
    std::memcpy(rawBuffer.data(), partCtx.ptr, partCtx.PendingBytes());
}

std::string_view DXBCPhysicalBlockRuntimeData::GetString(uint32_t offset) {
    return std::string_view(stringBuffer.data() + offset, std::strlen(stringBuffer.data() + offset));
}

uint32_t DXBCPhysicalBlockRuntimeData::InsertString(const std::string_view &str, StringSet &out) {
    // Zero is always mapped to a null terminated string
    if (!str.length()) {
        return 0;
    }
    
    // Deduplicate strings
    auto it = out.lookup.find(std::string(str));
    if (it != out.lookup.end()) {
        return it->second;
    }
    
    uint32_t offset = static_cast<uint32_t>(out.buffer.size());
    out.buffer.insert(out.buffer.end(), str.begin(), str.end());
    out.buffer.push_back('\0');

    out.lookup[std::string(str)] = offset;
    return offset;
}

uint32_t DXBCPhysicalBlockRuntimeData::InsertIndices(uint32_t *begin, uint32_t *end, IndexSet& out) {
    // TODO[rt]: Binary search instead of linear

    // Number of indices
    uint32_t count = static_cast<uint32_t>(std::distance(begin, end));

    // Deduplicate against the current range first
    for (uint32_t value : out.offsets) {
        if (out.buffer[value] != count) {
            continue;
        }

        // Matching range?
        if (!std::memcmp(out.buffer.data() + value + 1, begin, count * sizeof(uint32_t))) {
            return value;
        }
    } 
    
    uint32_t offset = static_cast<uint32_t>(out.buffer.size());
    out.buffer.push_back(count);
    out.buffer.insert(out.buffer.end(), begin, end);
    out.offsets.push_back(offset);
    return offset;
}

uint32_t DXBCPhysicalBlockRuntimeData::InsertRaw(const void *start, uint32_t length, std::vector<uint8_t> &out) {
    uint32_t offset = static_cast<uint32_t>(out.size());
    out.insert(out.end(), static_cast<const uint8_t*>(start), static_cast<const uint8_t*>(start) + length);
    return offset;
}

uint32_t DXBCPhysicalBlockRuntimeData::InsertSignatureElements(uint32_t signaturesOffset, StringSet& patchedStrings, IndexSet &patchedIndices) {
    uint32_t elementCount = indexBuffer[signaturesOffset];

    // Patch all elements
    for (uint32_t i = 0; i < elementCount; i++) {
        uint32_t elementIndex = indexBuffer[signaturesOffset + i + 1];

        // Patch common
        RecordEntry<DXBCRuntimeDataSignatureElement> &element = signatureElementRecords.records[elementIndex];
        element.record.semanticNameOffset = InsertString(GetString(element.record.semanticNameOffset), patchedStrings);

        // Patch indices
        uint32_t semanticIndicesCount = indexBuffer[element.record.semanticIndicesOffset];
        element.record.semanticIndicesOffset = InsertIndices(
            indexBuffer.data() + element.record.semanticIndicesOffset + 1,
            indexBuffer.data() + element.record.semanticIndicesOffset + 1 + semanticIndicesCount,
            patchedIndices
        );
    }

    // Insert back into indices
    return InsertIndices(
        indexBuffer.data() + signaturesOffset + 1,
        indexBuffer.data() + signaturesOffset + 1 + elementCount,
        patchedIndices
    );
}

void DXBCPhysicalBlockRuntimeData::ParseStringPart(DXBCParseContext &partCtx) {
    stringBuffer.resize(partCtx.PendingBytes() / sizeof(char));
    std::memcpy(stringBuffer.data(), partCtx.ptr, partCtx.PendingBytes());
}

template<typename T>
void DXBCPhysicalBlockRuntimeData::ParseTablePart(DXBCParseContext &partCtx, TablePart<T>& out) {
    out.header = partCtx.Consume<DXBCRuntimeDataTableHeader>();
    out.records.resize(partCtx.PendingBytes() / out.header.recordStride);
    
    for (uint64_t i = 0; i < out.records.size(); i++) {
        auto* dest = reinterpret_cast<uint8_t*>(&out.records[i].record);
        std::memcpy(dest, partCtx.ptr, out.header.recordStride);
        std::memset(dest + out.header.recordStride, 0, sizeof(T::record) - out.header.recordStride);
        partCtx.Skip(out.header.recordStride);
    }
}

template<typename T>
void DXBCPhysicalBlockRuntimeData::InsertTablePart(DXBCPhysicalBlock *block, const TablePart<T> &table, DXBCRuntimeDataPartType partType) {
    uint64_t headerOffset = block->stream.Append(DXBCRuntimeDataPartHeader {
        .type = partType
    });

    uint64_t start = block->stream.GetOffset();

    // Emit header
    block->stream.Append(DXBCRuntimeDataTableHeader {
        .recordCount = static_cast<uint32_t>(table.records.size()),
        .recordStride = table.header.recordStride
    });

    // Emit all records
    for (const T& entry : table.records) {
        block->stream.AppendData(&entry.record, table.header.recordStride);
    }

    // Fixup
    block->stream.AlignTo(sizeof(uint32_t));
    block->stream.GetMutableDataAt<DXBCRuntimeDataPartHeader>(headerOffset)->size = static_cast<uint32_t>(block->stream.GetOffset() - start);
}

void DXBCPhysicalBlockRuntimeData::Compile(const DXCompileJob &job) {
    // Block is optional
    DXBCPhysicalBlock *block = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::RuntimeData);
    if (!block) {
        return;
    }

    // Determine which functions have a local root signature and which do not
    // This is only known at compile time
    for (FunctionEntry& function : functionRecords.records) {
        for (uint32_t i = 0; i < job.instrumentationKey.localKeyCount; i++) {
            const ShaderLocalInstrumentationKey &key = job.instrumentationKey.localKeys[i];
            
            if (key.mangledName == function.name) {
                function.hasLocalRootSignature = key.localPhysicalMapping != nullptr;
                break;
            }
        }
    }

    // Compile resources
    CompileResources(job);
    CompileResourceVisibility(job);

    // Resources need to be relocated to maintain DXC ordering, this map holds the new indices
    std::vector<uint32_t> resourcePatchMap;
    resourcePatchMap.resize(resourceRecords.records.size());

    // Compiled string buffer, must start with null (so 0 -> empty string)
    StringSet patchedStrings;
    patchedStrings.buffer.push_back('\0');

    // Compiled raw buffer
    std::vector<uint8_t> patchedRawBuffer;

    // Compiled index buffer
    IndexSet patchedIndices;

    // The reordered resource records
    TablePart<ResourceEntry> patchedResourceRecords {
        .header = resourceRecords.header
    };

    // May not have had previous records
    if (!patchedResourceRecords.header.recordStride) {
        patchedResourceRecords.header.recordStride = sizeof(DXBCRuntimeDataResourceRecord);
    }
    
    // Emit resources and their data according to the emit order
    for (DXILShaderResourceClass _class : resourceEmitOrder) {
        ResourceBucket& bucket = resourceBuckets[static_cast<uint32_t>(_class)];
        
        for (uint32_t index : bucket.indices) {
            ResourceEntry& resource = resourceRecords.records[index];

            // Insert string
            resource.record.nameOffset = InsertString(resource.name, patchedStrings);

            // Set relocation
            resourcePatchMap[index] = static_cast<unsigned>(patchedResourceRecords.records.size());
            patchedResourceRecords.records.push_back(resource);
        }
    } 

    // Emit function data
    for (FunctionEntry& function : functionRecords.records) {
        function.record.nameOffset = InsertString(function.name, patchedStrings);
        function.record.unmangledNameOffset = InsertString(function.unmangledName, patchedStrings);

        // Patch the resources
        for (uint32_t& resource : function.resources) {
            resource = resourcePatchMap[resource];
        }

        // Always sorted, DXC rules!
        std::ranges::sort(function.resources);
        std::ranges::sort(function.dependencies);

        // Insert resource indices
        if (function.resources.size()) {
            function.record.resourceRecordIndex = InsertIndices(
                function.resources.data(),
                function.resources.data() + function.resources.size(),
                patchedIndices
            );
        }

        // Insert dependency indices
        if (function.dependencies.size()) {
            function.record.dependenciesStringIndex = InsertIndices(
                function.dependencies.data(),
                function.dependencies.data() + function.dependencies.size(),
                patchedIndices
            );
        }

        // Has revision2?
        if (functionRecords.header.recordStride >= sizeof(DXBCRuntimeDataFunctionRecord2)) {
            // May not be bound
            if (function.record.payload.rawShaderRef != UINT32_MAX) {
                // TODO[rt]: Validate this is right for Invalid kinds
                if (function.record.shaderKind == DXBCRuntimeDataShaderKind::Node) {
                    RecordEntry<DXBCRuntimeDataNodeShaderInfo> &node = nodeShaderInfoRecords.records[function.record.payload.nodeOffset];

                    // All index counts
                    uint32_t attributeCount = indexBuffer[node.record.attributesOffset];
                    uint32_t inputsCount    = indexBuffer[node.record.inputsOffset];
                    uint32_t outputsCount   = indexBuffer[node.record.outputsOffset];

                    // Patch all attributes
                    for (uint32_t i = 0; i < attributeCount; i++) {
                        uint32_t attributeIndex = indexBuffer[node.record.attributesOffset + i + 1];

                        // Patch payload data
                        RecordEntry<DXBCRuntimeDataNodeShaderFuncAttrib> &attribute = nodeShaderFuncAttribRecords.records[attributeIndex];
                        switch (attribute.record.attributeKind) {
                            default: {
                                break;
                            }
                            case DXBCRuntimeDataNodeFuncAttribKind::ID: {
                                auto& id = nodeIDRecords.records[attribute.record.payload.nodeIdOffset];
                                id.record.nameOffset = InsertString(GetString(id.record.nameOffset), patchedStrings);
                                break;
                            }
                            case DXBCRuntimeDataNodeFuncAttribKind::ThreadCount: {
                                uint32_t count = indexBuffer[attribute.record.payload.threadCountOffset];
                                attribute.record.payload.threadCountOffset = InsertIndices(
                                    indexBuffer.data() + attribute.record.payload.threadCountOffset + 1,
                                    indexBuffer.data() + attribute.record.payload.threadCountOffset + 1 + count,
                                    patchedIndices
                                );
                                break;
                            }
                            case DXBCRuntimeDataNodeFuncAttribKind::ShareInputOf: {
                                auto& id = nodeIDRecords.records[attribute.record.payload.shareInputOfOffset];
                                id.record.nameOffset = InsertString(GetString(id.record.nameOffset), patchedStrings);
                                break;
                            }
                            case DXBCRuntimeDataNodeFuncAttribKind::DispatchGrid: {
                                uint32_t count = indexBuffer[attribute.record.payload.dispatchGridOffset];
                                attribute.record.payload.dispatchGridOffset = InsertIndices(
                                    indexBuffer.data() + attribute.record.payload.dispatchGridOffset + 1,
                                    indexBuffer.data() + attribute.record.payload.dispatchGridOffset + 1 + count,
                                    patchedIndices
                                );
                                break;
                            }
                            case DXBCRuntimeDataNodeFuncAttribKind::MaxDispatchGrid: {
                                uint32_t count = indexBuffer[attribute.record.payload.dispatchGridOffset];
                                attribute.record.payload.dispatchGridOffset = InsertIndices(
                                    indexBuffer.data() + attribute.record.payload.dispatchGridOffset + 1,
                                    indexBuffer.data() + attribute.record.payload.dispatchGridOffset + 1 + count,
                                    patchedIndices
                                );
                                break;
                            }
                        }
                    }

                    // Patch all inputs
                    for (uint32_t i = 0; i < inputsCount; i++) {
                        uint32_t inputIndex = indexBuffer[node.record.inputsOffset + i + 1];

                        // Patch attribute data
                        RecordEntry<DXBCRuntimeDataNodeShaderIOAttrib> &attribute = nodeShaderIOAttribRecords.records[inputIndex];
                        switch (attribute.record.attributeKind) {
                            default: {
                                break;
                            }
                            case DXBCRuntimeDataNodeAttribKind::OutputID: {
                                auto& id = nodeIDRecords.records[attribute.record.payload.outputNodeIdOffset];
                                id.record.nameOffset = InsertString(GetString(id.record.nameOffset), patchedStrings);
                                break;
                            }
                        }
                    }

                    // Patch all outputs
                    for (uint32_t i = 0; i < outputsCount; i++) {
                        uint32_t outputIndex = indexBuffer[node.record.outputsOffset + i + 1];

                        // Patch attribute data
                        RecordEntry<DXBCRuntimeDataNodeShaderIOAttrib> &attribute = nodeShaderIOAttribRecords.records[outputIndex];
                        switch (attribute.record.attributeKind) {
                            default: {
                                break;
                            }
                            case DXBCRuntimeDataNodeAttribKind::OutputID: {
                                auto& id = nodeIDRecords.records[attribute.record.payload.outputNodeIdOffset];
                                id.record.nameOffset = InsertString(GetString(id.record.nameOffset), patchedStrings);
                                break;
                            }
                        }
                    }

                    // Rewrite attributes
                    node.record.attributesOffset = InsertIndices(
                        indexBuffer.data() + node.record.attributesOffset + 1,
                        indexBuffer.data() + node.record.attributesOffset + 1 + attributeCount,
                        patchedIndices
                    );

                    // Rewrite inputs
                    node.record.inputsOffset = InsertIndices(
                        indexBuffer.data() + node.record.inputsOffset + 1,
                        indexBuffer.data() + node.record.inputsOffset + 1 + inputsCount,
                        patchedIndices
                    );

                    // Rewrite outputs
                    node.record.outputsOffset = InsertIndices(
                        indexBuffer.data() + node.record.outputsOffset + 1,
                        indexBuffer.data() + node.record.outputsOffset + 1 + outputsCount,
                        patchedIndices
                    );
                } else {
                    // Otherwise a shader info, handle per case
                    switch (function.record.shaderKind) {
                        default: {
                            break;
                        }
                        case DXBCRuntimeDataShaderKind::Pixel: {
                            auto& psInfo = psInfoRecords.records[function.record.payload.rawShaderRef];
                            psInfo.record.signatureInputsOffset = InsertSignatureElements(psInfo.record.signatureInputsOffset, patchedStrings, patchedIndices);
                            psInfo.record.signatureOutputsOffset = InsertSignatureElements(psInfo.record.signatureOutputsOffset, patchedStrings, patchedIndices);
                            break;
                        }
                        case DXBCRuntimeDataShaderKind::Vertex: {
                            auto& vsInfo = vsInfoRecords.records[function.record.payload.rawShaderRef];
                            vsInfo.record.signatureInputsOffset = InsertSignatureElements(vsInfo.record.signatureInputsOffset, patchedStrings, patchedIndices);
                            vsInfo.record.signatureOutputsOffset = InsertSignatureElements(vsInfo.record.signatureOutputsOffset, patchedStrings, patchedIndices);
                            break;
                        }
                        case DXBCRuntimeDataShaderKind::Geometry: {
                            auto& gsInfo = gsInfoRecords.records[function.record.payload.rawShaderRef];
                            gsInfo.record.signatureInputsOffset = InsertSignatureElements(gsInfo.record.signatureInputsOffset, patchedStrings, patchedIndices);
                            gsInfo.record.signatureOutputsOffset = InsertSignatureElements(gsInfo.record.signatureOutputsOffset, patchedStrings, patchedIndices);
                            break;
                        }
                        case DXBCRuntimeDataShaderKind::Hull: {
                            auto& hsInfo = hsInfoRecords.records[function.record.payload.rawShaderRef];
                            hsInfo.record.signatureInputsOffset = InsertSignatureElements(hsInfo.record.signatureInputsOffset, patchedStrings, patchedIndices);
                            hsInfo.record.signatureOutputsOffset = InsertSignatureElements(hsInfo.record.signatureOutputsOffset, patchedStrings, patchedIndices);
                            hsInfo.record.signaturePatchOutputsOffset = InsertSignatureElements(hsInfo.record.signaturePatchOutputsOffset, patchedStrings, patchedIndices);
                            break;
                        }
                        case DXBCRuntimeDataShaderKind::Domain: {
                            auto& dsInfo = dsInfoRecords.records[function.record.payload.rawShaderRef];
                            dsInfo.record.signatureInputsOffset = InsertSignatureElements(dsInfo.record.signatureInputsOffset, patchedStrings, patchedIndices);
                            dsInfo.record.signatureOutputsOffset = InsertSignatureElements(dsInfo.record.signatureOutputsOffset, patchedStrings, patchedIndices);
                            dsInfo.record.signaturePatchInputsOffset = InsertSignatureElements(dsInfo.record.signaturePatchInputsOffset, patchedStrings, patchedIndices);
                            break;
                        }
                        case DXBCRuntimeDataShaderKind::Compute: {
                            auto& csInfo = csInfoRecords.records[function.record.payload.rawShaderRef];
                            uint32_t count = indexBuffer[csInfo.record.threadCountOffset];
                            csInfo.record.threadCountOffset = InsertIndices(
                                indexBuffer.data() + csInfo.record.threadCountOffset + 1,
                                indexBuffer.data() + csInfo.record.threadCountOffset + 1 + count,
                                patchedIndices
                            );
                            break;
                        }
                        case DXBCRuntimeDataShaderKind::Amplification: {
                            auto& asInfo = asInfoRecords.records[function.record.payload.rawShaderRef];
                            uint32_t count = indexBuffer[asInfo.record.threadCountOffset];
                            asInfo.record.threadCountOffset = InsertIndices(
                                indexBuffer.data() + asInfo.record.threadCountOffset + 1,
                                indexBuffer.data() + asInfo.record.threadCountOffset + 1 + count,
                                patchedIndices
                            );
                            break;
                        }
                    }
                }
            }
        }
    }

    // Emit subobject data
    for (SubObjectEntry& subObject : subObjectRecords.records) {
        subObject.record.nameOffset = InsertString(subObject.name, patchedStrings);

        // Emit the contained data
        switch (subObject.record.subObjectKind) {
            case DXBCRuntimeDataSubObjectKind::StateObjectConfig:
            case DXBCRuntimeDataSubObjectKind::RaytracingShaderConfig:
            case DXBCRuntimeDataSubObjectKind::RaytracingPipelineConfig:
            case DXBCRuntimeDataSubObjectKind::RaytracingPipelineConfig1: {
                break;
            }
            case DXBCRuntimeDataSubObjectKind::GlobalRootSignature:
            case DXBCRuntimeDataSubObjectKind::LocalRootSignature: {
                subObject.record.rootSignature.dataOffset = InsertRaw(
                    rawBuffer.data() +  subObject.record.rootSignature.dataOffset,
                    subObject.record.rootSignature.dataSize,
                    patchedRawBuffer
                );
                break;
            }
            case DXBCRuntimeDataSubObjectKind::SubObjectToExportsAssociation: {
                subObject.record.subObjectToExportsAssociation.subObjectStringOffset = InsertString(GetString(subObject.record.subObjectToExportsAssociation.exportsStringOffset), patchedStrings);

                // Total number of indices
                uint32_t indexCount = indexBuffer[subObject.record.subObjectToExportsAssociation.exportsStringOffset];

                // Copy over the strings
                TrivialStackVector<uint32_t, 8> exportIndices;
                for (uint32_t i = 0; i < indexCount; i++) {
                    std::string_view exportName = GetString(indexBuffer[subObject.record.subObjectToExportsAssociation.exportsStringOffset + i + 1]);
                    exportIndices.Add(InsertString(exportName, patchedStrings));
                }

                // Insert indices
                subObject.record.subObjectToExportsAssociation.exportsStringOffset = InsertIndices(
                    exportIndices.begin(),
                    exportIndices.end(),
                    patchedIndices
                );
                break;
            }
            case DXBCRuntimeDataSubObjectKind::HitGroup: {
                subObject.record.hitGroup.anyHitStringOffset = InsertString(GetString(subObject.record.hitGroup.anyHitStringOffset), patchedStrings);
                subObject.record.hitGroup.closestHitStringOffset = InsertString(GetString(subObject.record.hitGroup.closestHitStringOffset), patchedStrings);
                subObject.record.hitGroup.intersectionStringOffset = InsertString(GetString(subObject.record.hitGroup.intersectionStringOffset), patchedStrings);
                break;
            }
        }
    }

    // Determine number of parts
    header.partCount = 0;
    header.partCount += !patchedStrings.buffer.empty();
    header.partCount += !resourceRecords.records.empty();
    header.partCount += !functionRecords.records.empty();
    header.partCount += !patchedIndices.buffer.empty();
    header.partCount += !patchedRawBuffer.empty();
    header.partCount += !subObjectRecords.records.empty();
    header.partCount += !nodeIDRecords.records.empty();
    header.partCount += !nodeShaderIOAttribRecords.records.empty();
    header.partCount += !nodeShaderFuncAttribRecords.records.empty();
    header.partCount += !ioNodeRecords.records.empty();
    header.partCount += !nodeShaderInfoRecords.records.empty();
    header.partCount += !signatureElementRecords.records.empty();
    header.partCount += !vsInfoRecords.records.empty();
    header.partCount += !psInfoRecords.records.empty();
    header.partCount += !hsInfoRecords.records.empty();
    header.partCount += !dsInfoRecords.records.empty();
    header.partCount += !gsInfoRecords.records.empty();
    header.partCount += !csInfoRecords.records.empty();
    header.partCount += !msInfoRecords.records.empty();
    header.partCount += !asInfoRecords.records.empty();

    // Emit header, will be fixed up later
    block->stream.Append(header);

    /// Current part index
    uint32_t partIndex = 0;

    /// Offset after the header
    uint64_t partOffsets = block->stream.GetOffset();

    // Emit dummy offsets
    for (uint32_t i = 0; i < header.partCount; i++) {
        block->stream.Append<uint32_t>(0u);
    }

    // Emit all strings
    if (!patchedStrings.buffer.empty()) {
        block->stream.GetMutableDataAt<uint32_t>(partOffsets)[partIndex++] = block->stream.GetOffset();

        block->stream.Append(DXBCRuntimeDataPartHeader {
            .type = DXBCRuntimeDataPartType::String,
            .size = static_cast<uint32_t>(patchedStrings.buffer.size() + 3) & ~0x3
        });

        block->stream.AppendData(patchedStrings.buffer.data(), static_cast<uint32_t>(patchedStrings.buffer.size()));
        block->stream.AlignTo(sizeof(uint32_t));
    }

    // Emit all resources
    if (!patchedResourceRecords.records.empty()) {
        block->stream.GetMutableDataAt<uint32_t>(partOffsets)[partIndex++] = block->stream.GetOffset();
        InsertTablePart(block, patchedResourceRecords, DXBCRuntimeDataPartType::ResourceTable);
    }

    // Emit all functions
    if (!functionRecords.records.empty()) {
        block->stream.GetMutableDataAt<uint32_t>(partOffsets)[partIndex++] = block->stream.GetOffset();
        InsertTablePart(block, functionRecords, DXBCRuntimeDataPartType::FunctionTable);
    }

    // Emit all indices
    if (!patchedIndices.buffer.empty()) {
        block->stream.GetMutableDataAt<uint32_t>(partOffsets)[partIndex++] = block->stream.GetOffset();

        block->stream.Append(DXBCRuntimeDataPartHeader {
            .type = DXBCRuntimeDataPartType::IndexArray,
            .size = static_cast<uint32_t>(patchedIndices.buffer.size() * sizeof(uint32_t))
        });

        block->stream.AppendData(patchedIndices.buffer.data(), static_cast<uint32_t>(patchedIndices.buffer.size() * sizeof(uint32_t)));
        block->stream.AlignTo(sizeof(uint32_t));
    }

    // Emit all raw data
    if (!patchedRawBuffer.empty()) {
        block->stream.GetMutableDataAt<uint32_t>(partOffsets)[partIndex++] = block->stream.GetOffset();

        block->stream.Append(DXBCRuntimeDataPartHeader {
            .type = DXBCRuntimeDataPartType::RawBytes,
            .size = static_cast<uint32_t>(patchedRawBuffer.size() + 3) & ~0x3
        });
        
        block->stream.AppendData(patchedRawBuffer.data(), static_cast<uint32_t>(patchedRawBuffer.size()));
        block->stream.AlignTo(sizeof(uint32_t));
    }

    // Emit all subobjects
    if (!subObjectRecords.records.empty()) {
        block->stream.GetMutableDataAt<uint32_t>(partOffsets)[partIndex++] = block->stream.GetOffset();
        InsertTablePart(block, subObjectRecords, DXBCRuntimeDataPartType::SubObjectTable);
    }

    // Insert table records
    InsertRecordTablePart(block, nodeIDRecords, partOffsets, partIndex);
    InsertRecordTablePart(block, nodeShaderIOAttribRecords, partOffsets, partIndex);
    InsertRecordTablePart(block, nodeShaderFuncAttribRecords, partOffsets, partIndex);
    InsertRecordTablePart(block, ioNodeRecords, partOffsets, partIndex);
    InsertRecordTablePart(block, nodeShaderInfoRecords, partOffsets, partIndex);
    InsertRecordTablePart(block, signatureElementRecords, partOffsets, partIndex);
    InsertRecordTablePart(block, vsInfoRecords, partOffsets, partIndex);
    InsertRecordTablePart(block, psInfoRecords, partOffsets, partIndex);
    InsertRecordTablePart(block, hsInfoRecords, partOffsets, partIndex);
    InsertRecordTablePart(block, dsInfoRecords, partOffsets, partIndex);
    InsertRecordTablePart(block, gsInfoRecords, partOffsets, partIndex);
    InsertRecordTablePart(block, csInfoRecords, partOffsets, partIndex);
    InsertRecordTablePart(block, msInfoRecords, partOffsets, partIndex);
    InsertRecordTablePart(block, asInfoRecords, partOffsets, partIndex);
}

template<typename T>
void DXBCPhysicalBlockRuntimeData::InsertRecordTablePart(DXBCPhysicalBlock *block, const TablePart<RecordEntry<T>> &table, uint64_t partOffsets, uint32_t &partIndex) {
    // Parts never written if empty
    if (nodeIDRecords.records.empty()) {
        return;
    }
    
    block->stream.GetMutableDataAt<uint32_t>(partOffsets)[partIndex++] = block->stream.GetOffset();
    InsertTablePart(block, nodeIDRecords, T::kPartType);
}

void DXBCPhysicalBlockRuntimeData::AddResourceVisibility(const DXCompileJob& job, uint32_t index) {
    // By default, we load all resources, so they're all "visible"
    // Will be improved in the future
    for (FunctionEntry& function : functionRecords.records) {
        // If this is a local resource, and no LRS is present, skip it
        if (!function.hasLocalRootSignature && resourceRecords.records[index].isLocal) {
            continue;
        }

        // Visible!
        function.resources.push_back(index);
    }
}

void DXBCPhysicalBlockRuntimeData::CompileResources(const DXCompileJob &job) {
    // Get compiled binding info
    ASSERT(table.dxilModule, "PSV not supported for native DXBC");
    const DXILBindingInfo &dxil = table.dxilModule->GetBindingInfo();

    // Every resource prior to this is a user resource
    userResourcesEnd = resourceRecords.records.size();

    /** Emit all resources as per the metadata */

    resourceRecords.records.push_back(ResourceEntry {
        .record = DXBCRuntimeDataResourceRecord {
            ._class = static_cast<uint32_t>(DXILShaderResourceClass::UAVs),
            .shape = static_cast<uint32_t>(DXILShaderResourceShape::TypedBuffer),
            .id = dxil.global.shaderExportHandleId,
            .space = dxil.bindingInfo.global.space,
            .lower = dxil.bindingInfo.global.shaderExportBaseRegister,
            .upper = dxil.bindingInfo.global.shaderExportBaseRegister + (dxil.bindingInfo.global.shaderExportCount - 1u),
            .flags =  static_cast<uint32_t>(DXBCRuntimeDataResourceFlag::GloballyCoherent)
        },
        .name = "ShaderExport"
    });

    resourceRecords.records.push_back(ResourceEntry {
        .record = DXBCRuntimeDataResourceRecord {
            ._class = static_cast<uint32_t>(DXILShaderResourceClass::SRVs),
            .shape = static_cast<uint32_t>(DXILShaderResourceShape::TypedBuffer),
            .id = dxil.global.resourcePRMTHandleId,
            .space = dxil.bindingInfo.global.space,
            .lower = dxil.bindingInfo.global.resourcePRMTBaseRegister,
            .upper = dxil.bindingInfo.global.resourcePRMTBaseRegister,
            .flags =  0
        },
        .name = "ResourcePRMT"
    });

    resourceRecords.records.push_back(ResourceEntry {
        .record = DXBCRuntimeDataResourceRecord {
            ._class = static_cast<uint32_t>(DXILShaderResourceClass::SRVs),
            .shape = static_cast<uint32_t>(DXILShaderResourceShape::TypedBuffer),
            .id = dxil.global.samplerPRMTHandleId,
            .space = dxil.bindingInfo.global.space,
            .lower = dxil.bindingInfo.global.samplerPRMTBaseRegister,
            .upper = dxil.bindingInfo.global.samplerPRMTBaseRegister,
            .flags =  0
        },
        .name = "SamplerPRMT"
    });

    resourceRecords.records.push_back(ResourceEntry {
        .record = DXBCRuntimeDataResourceRecord {
            ._class = static_cast<uint32_t>(DXILShaderResourceClass::CBVs),
            .shape = static_cast<uint32_t>(DXILShaderResourceShape::CBuffer),
            .id = dxil.global.shaderDataConstantsHandleId,
            .space = dxil.bindingInfo.global.space,
            .lower = dxil.bindingInfo.global.shaderDataConstantRegister,
            .upper = dxil.bindingInfo.global.shaderDataConstantRegister,
            .flags =  0
        },
        .name = "CBufferConstantData"
    });

    resourceRecords.records.push_back(ResourceEntry {
        .record = DXBCRuntimeDataResourceRecord {
            ._class = static_cast<uint32_t>(DXILShaderResourceClass::CBVs),
            .shape = static_cast<uint32_t>(DXILShaderResourceShape::CBuffer),
            .id = dxil.global.descriptorConstantsHandleId,
            .space = dxil.bindingInfo.global.space,
            .lower = dxil.bindingInfo.global.descriptorConstantBaseRegister,
            .upper = dxil.bindingInfo.global.descriptorConstantBaseRegister,
            .flags =  0
        },
        .name = "CBufferDescriptorData"
    });

    if (job.instrumentationKey.localKeys) {
        resourceRecords.records.push_back(ResourceEntry {
            .record = DXBCRuntimeDataResourceRecord {
                ._class = static_cast<uint32_t>(DXILShaderResourceClass::CBVs),
                .shape = static_cast<uint32_t>(DXILShaderResourceShape::CBuffer),
                .id = dxil.local.descriptorConstantsHandleId,
                .space = dxil.bindingInfo.local.space,
                .lower = dxil.bindingInfo.local.descriptorConstantBaseRegister,
                .upper = dxil.bindingInfo.local.descriptorConstantBaseRegister,
                .flags =  0
            },
            .isLocal = true,
            .name = "CBufferDescriptorDataLocal"
        });
    }

    resourceRecords.records.push_back(ResourceEntry {
        .record = DXBCRuntimeDataResourceRecord {
            ._class = static_cast<uint32_t>(DXILShaderResourceClass::CBVs),
            .shape = static_cast<uint32_t>(DXILShaderResourceShape::CBuffer),
            .id = dxil.global.eventConstantsHandleId,
            .space = dxil.bindingInfo.global.space,
            .lower = dxil.bindingInfo.global.eventConstantBaseRegister,
            .upper = dxil.bindingInfo.global.eventConstantBaseRegister,
            .flags =  0
        },
        .name = "CBufferEventData"
    });
    
    // Get shader data
    IL::ShaderDataMap& shaderDataMap = table.dxilModule->GetProgram()->GetShaderDataMap();

    // Current data offset
    uint32_t dataOffset{0};

    // Emit all shader datas
    for (auto it = shaderDataMap.begin(); it != shaderDataMap.end(); it++) {
        if (!(it->type & ShaderDataType::DescriptorMask)) {
            continue;
        }

        resourceRecords.records.push_back(ResourceEntry {
            .record = DXBCRuntimeDataResourceRecord {
                ._class = static_cast<uint32_t>(DXILShaderResourceClass::UAVs),
                .shape = static_cast<uint32_t>(DXILShaderResourceShape::TypedBuffer),
                .id = dxil.global.shaderDataHandleId + dataOffset,
                .space = dxil.bindingInfo.global.space,
                .lower = dxil.bindingInfo.global.shaderResourceBaseRegister + dataOffset,
                .upper = dxil.bindingInfo.global.shaderResourceBaseRegister + dataOffset,
                .flags =  static_cast<uint32_t>(DXBCRuntimeDataResourceFlag::GloballyCoherent)
            },
            .name = "ShaderResource"
        });

        // Next
        dataOffset++;
    }

    // Reset offset for user bindings
    dataOffset = 0;

    // Emit all shader bindings
    for (auto it = shaderDataMap.begin(); it != shaderDataMap.end(); it++) {
        if (!(it->type & ShaderDataType::BindingMask)) {
            continue;
        }

        // Destination class
        DXILShaderResourceClass targetClass = it->bufferBinding.isWritable ? DXILShaderResourceClass::UAVs : DXILShaderResourceClass::SRVs;

        resourceRecords.records.push_back(ResourceEntry {
            .record = DXBCRuntimeDataResourceRecord {
                ._class = static_cast<uint32_t>(targetClass),
                .shape = static_cast<uint32_t>(DXILShaderResourceShape::TypedBuffer),
                .id = dxil.bindings.shaderDataBindingHandleIds[static_cast<uint32_t>(targetClass)] + dataOffset,
                .space = dxil.bindingInfo.bindings.space,
                .lower = dxil.bindingInfo.bindings.shaderBindingResourceBaseRegister + dataOffset,
                .upper = dxil.bindingInfo.bindings.shaderBindingResourceBaseRegister + dataOffset,
                .flags =  0
            },
            .name = "ShaderResource"
        });

        // Next
        dataOffset++;
    }
}

void DXBCPhysicalBlockRuntimeData::CompileResourceVisibility(const DXCompileJob& job) {
    // Categorize resources
    // New ones will appear last
    for (uint64_t i = 0; i < resourceRecords.records.size(); i++) {
        ResourceEntry& entry = resourceRecords.records[i];
        resourceBuckets[entry.record._class].indices.push_back(static_cast<uint32_t>(i));
    }

    // Append all new indices in standard RDAT order
    for (DXILShaderResourceClass _class : resourceEmitOrder) {
        ResourceBucket& bucket = resourceBuckets[static_cast<uint32_t>(_class)];
        
        for (uint32_t index : bucket.indices) {
            if (index >= userResourcesEnd) {
                AddResourceVisibility(job, index);
            }
        } 
    }
}

void DXBCPhysicalBlockRuntimeData::CopyTo(DXBCPhysicalBlockRuntimeData &out) {
    out.header = header;
    out.parts = parts;
    out.resourceRecords = resourceRecords;
    out.functionRecords = functionRecords;
    out.subObjectRecords = subObjectRecords;
    out.nodeIDRecords = nodeIDRecords;
    out.nodeShaderIOAttribRecords = nodeShaderIOAttribRecords;
    out.nodeShaderFuncAttribRecords = nodeShaderFuncAttribRecords;
    out.ioNodeRecords = ioNodeRecords;
    out.nodeShaderInfoRecords = nodeShaderInfoRecords;
    out.signatureElementRecords = signatureElementRecords;
    out.vsInfoRecords = vsInfoRecords;
    out.psInfoRecords = psInfoRecords;
    out.hsInfoRecords = hsInfoRecords;
    out.dsInfoRecords = dsInfoRecords;
    out.gsInfoRecords = gsInfoRecords;
    out.csInfoRecords = csInfoRecords;
    out.msInfoRecords = msInfoRecords;
    out.asInfoRecords = asInfoRecords;
    out.indexBuffer = indexBuffer;
    out.rawBuffer = rawBuffer;
    out.stringBuffer = stringBuffer;
    out.userResourcesEnd = userResourcesEnd;
    out.header = header;

    for (uint32_t i = 0; i < static_cast<uint32_t>(DXILShaderResourceClass::Count); i++) {
        out.resourceBuckets[i] = resourceBuckets[i];
    }
}