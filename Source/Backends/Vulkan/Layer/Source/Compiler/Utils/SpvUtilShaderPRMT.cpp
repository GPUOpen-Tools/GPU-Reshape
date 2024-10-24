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

#include <Backends/Vulkan/Compiler/Utils/SpvUtilShaderPRMT.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Compiler/SpvJob.h>
#include <Backends/Vulkan/Resource/DescriptorData.h>

// Backend
#include <Backend/IL/ResourceTokenMetadataField.h>
#include <Backend/IL/ResourceTokenPacking.h>
#include <Backend/IL/ID.h>

// Common
#include <Common/Containers/TrivialStackVector.h>

SpvUtilShaderPRMT::SpvUtilShaderPRMT(const Allocators &allocators, IL::Program &program, SpvPhysicalBlockTable &table) :
    allocators(allocators),
    program(program),
    table(table) {
}

void SpvUtilShaderPRMT::CompileRecords(const SpvJob &job) {
    // Capability set
    table.capability.Add(SpvCapabilityImageBuffer);

    // IL type map
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();

    // UInt32
    const Backend::IL::Type *uintType = ilTypeMap.FindTypeOrAdd(Backend::IL::IntType{
        .bitWidth = 32,
        .signedness = false
    });

    // Buffer<uint>
    buffer32UI = ilTypeMap.FindTypeOrAdd(Backend::IL::BufferType{
        .elementType = uintType,
        .samplerMode = Backend::IL::ResourceSamplerMode::Compatible,
        .texelType = Backend::IL::Format::R32UInt
    });

    // Buffer<uint>*
    buffer32UIPtr = ilTypeMap.FindTypeOrAdd(Backend::IL::PointerType{
        .pointee = buffer32UI,
        .addressSpace = Backend::IL::AddressSpace::Resource
    });

    // Control structure for a resource mapping
    prmControl = ilTypeMap.FindTypeOrAdd(Backend::IL::ArrayType {
        .elementType = uintType,
        .count = 3
    });

    // Id allocations
    prmTableId = table.scan.header.bound++;

    // SpvIds
    SpvId buffer32UIPtrId = table.typeConstantVariable.typeMap.GetSpvTypeId(buffer32UIPtr);

    // PRMT
    SpvInstruction &spvCounterVar = table.typeConstantVariable.block->stream.Allocate(SpvOpVariable, 4);
    spvCounterVar[1] = buffer32UIPtrId;
    spvCounterVar[2] = prmTableId;
    spvCounterVar[3] = SpvStorageClassUniformConstant;

    // Descriptor set
    SpvInstruction &spvCounterSet = table.annotation.block->stream.Allocate(SpvOpDecorate, 4);
    spvCounterSet[1] = prmTableId;
    spvCounterSet[2] = SpvDecorationDescriptorSet;
    spvCounterSet[3] = job.instrumentationKey.pipelineLayoutUserSlots;

    // Binding
    SpvInstruction &spvCounterBinding = table.annotation.block->stream.Allocate(SpvOpDecorate, 4);
    spvCounterBinding[1] = prmTableId;
    spvCounterBinding[2] = SpvDecorationBinding;
    spvCounterBinding[3] = job.bindingInfo.prmtDescriptorOffset;

    // Add to all entry points
    table.entryPoint.AddInterface(SpvStorageClassUniform, prmTableId);
}

SpvUtilShaderPRMT::SpvPRMTOffset SpvUtilShaderPRMT::GetResourcePRMTOffset(const SpvJob& job, SpvStream &stream, IL::ID function, IL::ID resource) {
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();

    // Output
    SpvPRMTOffset out;

    // Identifiers
    out.metadataOffset = table.scan.header.bound++;
    out.descriptorOffset = table.scan.header.bound++;
    out.tableNotBound = table.scan.header.bound++;

    // UInt32
    const Backend::IL::Type *uintType = ilTypeMap.FindTypeOrAdd(Backend::IL::IntType {
        .bitWidth = 32,
        .signedness = false
    });

    // Get decoration
    DynamicSpvValueDecoration valueDecoration = GetSourceResourceDecoration(job, stream, function, resource);

    // Get the descriptor fed offset into the PRM table
    IL::ID baseOffsetId = table.shaderDescriptorConstantData.GetDescriptorData(stream, valueDecoration.descriptorSet, kDescriptorDataOffsetDWord);

    // PRM index, DescriptorSetOffset + BindingOffset
    SpvInstruction& spv = stream.Allocate(SpvOpIAdd, 5);
    spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
    spv[2] = out.descriptorOffset;
    spv[3] = baseOffsetId;
    spv[4] = valueDecoration.dynamicOffset;
    
    // Stride id
    IL::ID strideId = table.scan.header.bound++;

    // Null value
    SpvInstruction &spvOffset = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
    spvOffset[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType {.bitWidth = 32, .signedness = false}));
    spvOffset[2] = strideId;
    spvOffset[3] = static_cast<uint32_t>(Backend::IL::ResourceTokenMetadataField::Count);
    
    // Final PRM offset, Offset * MetadataStride
    SpvInstruction& mulSpv = stream.Allocate(SpvOpIMul, 5);
    mulSpv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
    mulSpv[2] = out.metadataOffset;
    mulSpv[3] = out.descriptorOffset;
    mulSpv[4] = strideId;

    // Requires bounds checking?
    if (valueDecoration.checkOutOfBounds) {
        // Allocate identifier
        out.outOfBounds = table.scan.header.bound++;

        // Read length of the segment
        IL::ID baseLengthId = table.shaderDescriptorConstantData.GetDescriptorData(stream, valueDecoration.descriptorSet, kDescriptorDataLengthDWord);

        // DynamicOffset >= Length
        SpvInstruction& spv = stream.Allocate(SpvOpUGreaterThanEqual, 5);
        spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(program.GetTypeMap().FindTypeOrAdd(Backend::IL::BoolType { }));
        spv[2] = out.outOfBounds;
        spv[3] = valueDecoration.dynamicOffset;
        spv[4] = baseLengthId;
    }

    // Validate table binding
    {
        // Null constant id
        IL::ID nullId = table.scan.header.bound++;

        // Null value
        SpvInstruction &spvOffset = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
        spvOffset[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType {.bitWidth = 32, .signedness = false}));
        spvOffset[2] = nullId;
        spvOffset[3] = kDescriptorDataNullOffset;

        // BaseOffset == kDescriptorDataNullOffset
        SpvInstruction& spv = stream.Allocate(SpvOpIEqual, 5);
        spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(program.GetTypeMap().FindTypeOrAdd(Backend::IL::BoolType { }));
        spv[2] = out.tableNotBound;
        spv[3] = baseOffsetId;
        spv[4] = nullId;
    }
    
    // OK
    return out;
}

void SpvUtilShaderPRMT::GetToken(const SpvJob& job, SpvStream &stream, IL::ID function, IL::ID resource, IL::ID result) {
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();

    // Total number of dwords in the token metadata
    static constexpr uint32_t kMetadataDWordCount = static_cast<uint32_t>(Backend::IL::ResourceTokenMetadataField::Count);
    
    // UInt32
    Backend::IL::IntType typeInt;
    typeInt.bitWidth = 32;
    typeInt.signedness = false;
    const Backend::IL::Type *uintType = ilTypeMap.FindTypeOrAdd(typeInt);

    // <UInt32, 4>
    const Backend::IL::Type *uint4Type = ilTypeMap.FindTypeOrAdd(Backend::IL::VectorType{
        .containedType = uintType,
        .dimension = 4
    });

    // Identifiers
    uint32_t bufferId = table.scan.header.bound++;

    // Get offset
    SpvPRMTOffset mappingOffset = GetResourcePRMTOffset(job, stream, function, resource);

    // SpvIds
    uint32_t buffer32UIId = table.typeConstantVariable.typeMap.GetSpvTypeId(buffer32UI);
    uint32_t uintTypeId = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
    uint32_t uint4TypeId = table.typeConstantVariable.typeMap.GetSpvTypeId(uint4Type);
    uint32_t tokenMetadataStructId = table.typeConstantVariable.typeMap.GetSpvTypeId(program.GetTypeMap().GetResourceToken());

    // Load buffer
    SpvInstruction& spvLoad = stream.Allocate(SpvOpLoad, 4);
    spvLoad[1] = buffer32UIId;
    spvLoad[2] = bufferId;
    spvLoad[3] = prmTableId;
    
    // Stride id
    IL::ID strideId = table.scan.header.bound++;

    // Null value
    SpvInstruction &spvOffset = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
    spvOffset[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType {.bitWidth = 32, .signedness = false}));
    spvOffset[2] = strideId;
    spvOffset[3] = 1u;

    // All metadata
    TrivialStackVector<SpvId, kMetadataDWordCount> metadataMap;

    // Current dword texel offset
    IL::ID texelOffsetId = mappingOffset.metadataOffset;

    // Load all dwords
    for (uint32_t i = 0; i < kMetadataDWordCount; i++) {
        uint32_t loadId = table.scan.header.bound++;
        uint32_t fieldTexelId = table.scan.header.bound++;

        // Increment all successive offsets
        if (i != 0) {
            uint32_t fieldOffset = table.scan.header.bound++;

            // Offset + 1
            SpvInstruction& spv = stream.Allocate(SpvOpIAdd, 5);
            spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
            spv[2] = fieldOffset;
            spv[3] = texelOffsetId;
            spv[4] = strideId;
            texelOffsetId = fieldOffset;
        }

        // Load the texel (4)
        SpvInstruction& spvTexel = stream.Allocate(SpvOpImageFetch, 5);
        spvTexel[1] = uint4TypeId;
        spvTexel[2] = loadId;
        spvTexel[3] = bufferId;
        spvTexel[4] = texelOffsetId;

        // Extract first value
        SpvInstruction& spvExtract = stream.Allocate(SpvOpCompositeExtract, 5);
        spvExtract[1] = uintTypeId;
        spvExtract[2] = fieldTexelId;
        spvExtract[3] = loadId;
        spvExtract[4] = 0;
        metadataMap.Add(fieldTexelId);
    }

    // Requires out-of-bounds selection?
    if (mappingOffset.outOfBounds != IL::InvalidID) {
        uint32_t outOfBoundsConstantId = table.scan.header.bound++;
        uint32_t oobValidationId = table.scan.header.bound++;

        // Special out-of-bounds value
        SpvInstruction &spvZero = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
        spvZero[1] = uintTypeId;
        spvZero[2] = outOfBoundsConstantId;
        spvZero[3] = IL::kResourceTokenPUIDInvalidOutOfBounds;

        // Select out-of-bounds value if needed
        SpvInstruction& spvSelect = stream.Allocate(SpvOpSelect, 6);
        spvSelect[1] = uintTypeId;
        spvSelect[2] = oobValidationId;
        spvSelect[3] = mappingOffset.outOfBounds;
        spvSelect[4] = outOfBoundsConstantId;
        spvSelect[5] = metadataMap[0];
        metadataMap[0] = oobValidationId;
    }

    // Table binding selection
    {
        uint32_t packedTokenId = table.scan.header.bound++;
        uint32_t tableNotBoundConstantId = table.scan.header.bound++;

        // Special table-not-bound value
        SpvInstruction &spvZero = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
        spvZero[1] = uintTypeId;
        spvZero[2] = tableNotBoundConstantId;
        spvZero[3] = IL::kResourceTokenPUIDInvalidTableNotBound;

        // Select table-not-bound value if needed
        SpvInstruction& spvSelect = stream.Allocate(SpvOpSelect, 6);
        spvSelect[1] = uintTypeId;
        spvSelect[2] = packedTokenId;
        spvSelect[3] = mappingOffset.tableNotBound;
        spvSelect[4] = tableNotBoundConstantId;
        spvSelect[5] = metadataMap[0];
        metadataMap[0] = packedTokenId;
    }

    // Validate we got the expected number of dwords
    ASSERT(metadataMap.Size() == kMetadataDWordCount, "Metadata count mismatch");

    // Create a composite representing all these dwords
    SpvInstruction& spvConstruct = stream.Allocate(SpvOpCompositeConstruct, 3 + kMetadataDWordCount);
    spvConstruct[1] = tokenMetadataStructId;
    spvConstruct[2] = result;

    // Fill dwords
    for (uint32_t i = 0; i < kMetadataDWordCount; i++) {
        spvConstruct[3 + i] = metadataMap[i];
    }
}

SpvUtilShaderPRMT::DynamicSpvValueDecoration SpvUtilShaderPRMT::GetSourceResourceDecoration(const SpvJob& job, SpvStream& stream, IL::ID function, IL::ID resource) {
    // Source binding?
    if (table.annotation.IsDecoratedBinding(resource)) {
        const SpvValueDecoration& source = table.annotation.GetDecoration(resource);

        // Create dynamic ids
        DynamicSpvValueDecoration dynamic;
        dynamic.descriptorSet = table.scan.header.bound++;
        dynamic.descriptorOffset = table.scan.header.bound++;
        dynamic.dynamicOffset = table.scan.header.bound++;

        // Get the sets physical mapping
        const DescriptorLayoutPhysicalMapping& descriptorSetPhysicalMapping = job.instrumentationKey.physicalMapping->descriptorSets.at(source.descriptorSet);

        // Get the statically known PRMT offset for this binding (descriptor offsets are always the bindings)
        const uint32_t prmtOffset = descriptorSetPhysicalMapping.bindings.at(source.descriptorOffset).prmtOffset;

        // uint32
        SpvId uintId = table.typeConstantVariable.typeMap.GetSpvTypeId(program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType {.bitWidth = 32, .signedness = false}));

        // Descriptor Set
        SpvInstruction &spvDescSet = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
        spvDescSet[1] = uintId;
        spvDescSet[2] = dynamic.descriptorSet;
        spvDescSet[3] = source.descriptorSet;

        // Descriptor Offset
        SpvInstruction &spvDescOffset = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
        spvDescOffset[1] = uintId;
        spvDescOffset[2] = dynamic.descriptorOffset;
        spvDescOffset[3] = source.descriptorOffset;

        // PRMT Offset
        SpvInstruction &spvOffset = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
        spvOffset[1] = uintId;
        spvOffset[2] = dynamic.dynamicOffset;
        spvOffset[3] = prmtOffset;

        // OK
        return dynamic;
    }

    // Get the identifier map
    IL::IdentifierMap& idMap = table.program.GetIdentifierMap();

    // Are we referencing an instruction?
    if (auto ref = IL::InstructionRef<>(idMap.Get(resource)); ref) {
        // Get originating instruction
        const IL::Instruction* _instr = ref.Get();

        // Handle type
        switch (_instr->opCode) {
            default: {
                ASSERT(false, "Backtracking not implemented for op-code");
                return {};
            }
            case IL::OpCode::Unexposed: {
                auto* unexposed = _instr->As<IL::UnexposedInstruction>();

                // Handle unexposed type
                const SpvInstruction* instr = table.function.block->stream.GetInstruction(unexposed->source);
                switch (instr->GetOp()) {
                    default: {
                        ASSERT(false, "Backtracking not implemented for unexposed p-code");
                        return {};
                    }
                    case SpvOpImage: {
                        // Get from sampled image operand
                        return GetSourceResourceDecoration(job, stream, function, instr->Word(3));
                    }
                    case SpvOpCopyObject: {
                        // Get from source operand
                        return GetSourceResourceDecoration(job, stream, function, instr->Word(3));
                    }
                    case SpvOpSampledImage: {
                        // Get from base image
                        return GetSourceResourceDecoration(job, stream, function, instr->Word(3));
                    }
                }
            }
            case IL::OpCode::AddressChain: {
                auto* addressChain = _instr->As<IL::AddressChainInstruction>();
                ASSERT(addressChain->chains.count == 2, "Unexpected address chain on descriptor data");

                // Get base decoration
                DynamicSpvValueDecoration dynamic = GetSourceResourceDecoration(job, stream, function, addressChain->composite);

                // Address could be constant, but easier to just assume dynamic indexing at this point
                dynamic.checkOutOfBounds = true;

                // Allocate id
                IL::ID offsetId = table.scan.header.bound++;

                // Add dynamic offset
                SpvInstruction& spv = stream.Allocate(SpvOpIAdd, 5);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType {.bitWidth = 32, .signedness = false}));
                spv[2] = offsetId;
                spv[3] = dynamic.dynamicOffset;
                spv[4] = addressChain->chains[1].index;

                // Set new offset
                dynamic.dynamicOffset = offsetId;

                // OK
                return dynamic;
            }
            case IL::OpCode::Load: {
                return GetSourceResourceDecoration(job, stream, function, _instr->As<IL::LoadInstruction>()->address);
            }
        }
    }

    // Are we referencing a parameter?
    if (const Backend::IL::Variable *var = program.GetFunctionList().GetFunction(function)->GetParameters().GetVariable(resource)) {
        // Get metadata
        const SpvPhysicalBlockFunction::IdentifierMetadata& fnMd  = table.function.identifierMetadata[function];
        const SpvPhysicalBlockFunction::IdentifierMetadata& varMd = table.function.identifierMetadata[var->id];

        // Allocate dynamic ids
        DynamicSpvValueDecoration dynamic;
        dynamic.descriptorSet = table.scan.header.bound++;
        dynamic.descriptorOffset = table.scan.header.bound++;
        dynamic.dynamicOffset = table.scan.header.bound++;

        // uint32
        uint32_t uintTypeId = table.typeConstantVariable.typeMap.GetSpvTypeId(program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{}));

        // Extract descriptor set
        SpvInstruction& spvExDescSet = stream.Allocate(SpvOpCompositeExtract, 6);
        spvExDescSet[1] = uintTypeId;
        spvExDescSet[2] = dynamic.descriptorSet;
        spvExDescSet[3] = fnMd.function.optionalControlStructure;
        spvExDescSet[4] = varMd.parameter.linearIndex;
        spvExDescSet[5] = 0;
        
        // Extract descriptor offset
        SpvInstruction& spvExDescOffset = stream.Allocate(SpvOpCompositeExtract, 6);
        spvExDescOffset[1] = uintTypeId;
        spvExDescOffset[2] = dynamic.descriptorOffset;
        spvExDescOffset[3] = fnMd.function.optionalControlStructure;
        spvExDescOffset[4] = varMd.parameter.linearIndex;
        spvExDescOffset[5] = 1;
        
        // Extract PRMT offset
        SpvInstruction& spvExPRMOffset = stream.Allocate(SpvOpCompositeExtract, 6);
        spvExPRMOffset[1] = uintTypeId;
        spvExPRMOffset[2] = dynamic.dynamicOffset;
        spvExPRMOffset[3] = fnMd.function.optionalControlStructure;
        spvExPRMOffset[4] = varMd.parameter.linearIndex;
        spvExPRMOffset[5] = 2;

        return dynamic;
    }

    // We've hit an unsupported path
    ASSERT(false, "Unsupported path");
    return {};
}

IL::ID SpvUtilShaderPRMT::CreatePRMControl(const SpvJob &job, SpvStream &stream, IL::ID function, IL::ID resource) {
    IL::ID id = table.scan.header.bound++;

    // Backtrack the resource decoration
    DynamicSpvValueDecoration decoration = GetSourceResourceDecoration(job, stream, function, resource);

    // Create the control structure
    SpvInstruction& spvConstruct = stream.Allocate(SpvOpCompositeConstruct, 6);
    spvConstruct[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(prmControl);
    spvConstruct[2] = id;
    spvConstruct[3] = decoration.descriptorSet;
    spvConstruct[4] = decoration.descriptorOffset;
    spvConstruct[5] = decoration.dynamicOffset;

    // OK
    return id;
}

void SpvUtilShaderPRMT::CopyTo(SpvPhysicalBlockTable &remote, SpvUtilShaderPRMT &out) {
    out.prmTableId = prmTableId;
    out.buffer32UI = buffer32UI;
    out.buffer32UIPtr = buffer32UIPtr;
    out.prmControl = prmControl;
}
