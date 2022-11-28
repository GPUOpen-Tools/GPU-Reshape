#include <Backends/Vulkan/Compiler/Utils/SpvUtilShaderPRMT.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Compiler/SpvJob.h>

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
    const Backend::IL::Type *intType = ilTypeMap.FindTypeOrAdd(Backend::IL::IntType{
        .bitWidth = 32,
        .signedness = false
    });

    // Buffer<uint>
    buffer32UI = ilTypeMap.FindTypeOrAdd(Backend::IL::BufferType{
        .elementType = intType,
        .samplerMode = Backend::IL::ResourceSamplerMode::Compatible,
        .texelType = Backend::IL::Format::R32UInt
    });

    // Buffer<uint>*
    buffer32UIPtr = ilTypeMap.FindTypeOrAdd(Backend::IL::PointerType{
        .pointee = buffer32UI,
        .addressSpace = Backend::IL::AddressSpace::Resource,
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
}

IL::ID SpvUtilShaderPRMT::GetResourcePRMTOffset(SpvStream &stream, IL::ID resource) {
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();

    // Identifiers
    uint32_t prmtOffsetId = table.scan.header.bound++;
    uint32_t offsetId = table.scan.header.bound++;

    // UInt32
    const Backend::IL::Type *uintType = ilTypeMap.FindTypeOrAdd(Backend::IL::IntType {
        .bitWidth = 32,
        .signedness = false
    });

    // Get decoration
    SpvValueDecoration valueDecoration = GetSourceResourceDecoration(resource);

    // Get the descriptor fed offset into the PRM table
    IL::ID baseOffsetId = table.shaderDescriptorConstantData.GetDescriptorOffset(stream, valueDecoration.descriptorSet);

    // Offset
    SpvInstruction &spvZero = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
    spvZero[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
    spvZero[2] = offsetId;
    spvZero[3] = valueDecoration.offset;

    // Final PRM index, DescriptorSetOffset + BindingOffset
    SpvInstruction& spv = stream.Allocate(SpvOpIAdd, 5);
    spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
    spv[2] = prmtOffsetId;
    spv[3] = baseOffsetId;
    spv[4] = offsetId;

    // OK
    return prmtOffsetId;
}

void SpvUtilShaderPRMT::GetToken(SpvStream &stream, IL::ID resource, IL::ID result) {
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();

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
    uint32_t texelId = table.scan.header.bound++;

    // Get offset
    IL::ID mappingOffset = GetResourcePRMTOffset(stream, resource);

    // SpvIds
    uint32_t buffer32UIId = table.typeConstantVariable.typeMap.GetSpvTypeId(buffer32UI);
    uint32_t uintTypeId = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
    uint32_t uint4TypeId = table.typeConstantVariable.typeMap.GetSpvTypeId(uint4Type);

    // Load buffer
    SpvInstruction& spvLoad = stream.Allocate(SpvOpLoad, 4);
    spvLoad[1] = buffer32UIId;
    spvLoad[2] = bufferId;
    spvLoad[3] = prmTableId;

    // Fetch texel
    SpvInstruction& spvTexel = stream.Allocate(SpvOpImageFetch, 5);
    spvTexel[1] = uint4TypeId;
    spvTexel[2] = texelId;
    spvTexel[3] = bufferId;
    spvTexel[4] = mappingOffset;

    // Fetch texel
    SpvInstruction& spvExtract = stream.Allocate(SpvOpCompositeExtract, 5);
    spvExtract[1] = uintTypeId;
    spvExtract[2] = result;
    spvExtract[3] = texelId;
    spvExtract[4] = 0;
}

SpvValueDecoration SpvUtilShaderPRMT::GetSourceResourceDecoration(IL::ID resource) {
    if (table.annotation.IsDecoratedBinding(resource)) {
        return table.annotation.GetDecoration(resource);
    }

    // Get the identifier map
    IL::IdentifierMap& idMap = table.program.GetIdentifierMap();

    // Get originating instruction
    const IL::Instruction* ref = IL::InstructionRef<>(idMap.Get(resource)).Get();

    // Handle type
    switch (ref->opCode) {
        default:
            ASSERT(false, "Backtracking not implemented for op-code");
            return {};
        case IL::OpCode::Load:
            return GetSourceResourceDecoration(ref->As<IL::LoadInstruction>()->address);
    }
}

void SpvUtilShaderPRMT::CopyTo(SpvPhysicalBlockTable &remote, SpvUtilShaderPRMT &out) {
    out.prmTableId = prmTableId;
    out.buffer32UI = buffer32UI;
    out.buffer32UIPtr = buffer32UIPtr;
}
