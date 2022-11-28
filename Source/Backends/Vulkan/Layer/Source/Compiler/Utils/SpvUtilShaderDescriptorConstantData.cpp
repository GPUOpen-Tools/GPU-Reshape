#include <Backends/Vulkan/Compiler/Utils/SpvUtilShaderDescriptorConstantData.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Compiler/SpvJob.h>

SpvUtilShaderDescriptorConstantData::SpvUtilShaderDescriptorConstantData(const Allocators &allocators, IL::Program &program, SpvPhysicalBlockTable &table) :
    allocators(allocators),
    program(program),
    table(table) {

}

void SpvUtilShaderDescriptorConstantData::CompileRecords(const SpvJob &job) {
    // Capability set
    table.capability.Add(SpvCapabilityImageBuffer);

    // IL type map
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();

    // UInt32
    const Backend::IL::Type *intType = ilTypeMap.FindTypeOrAdd(Backend::IL::IntType{
        .bitWidth = 32,
        .signedness = false
    });

    // <UInt32, 4>[N]
    const Backend::IL::ArrayType* arrayType = ilTypeMap.FindTypeOrAdd(Backend::IL::ArrayType{
        .elementType = ilTypeMap.FindTypeOrAdd(Backend::IL::VectorType{
            .containedType = intType,
            .dimension = 4
        }),

        // Snap array to 4 (row boundary)
        .count = std::max<uint32_t>(1, (job.instrumentationKey.pipelineLayoutUserSlots + 3) / 4)
    });

    // { <UInt32, 4>[N] }*
    const Backend::IL::PointerType *int4DataTypeStructPtr = ilTypeMap.FindTypeOrAdd(Backend::IL::PointerType{
        .pointee = ilTypeMap.FindTypeOrAdd(Backend::IL::StructType{
            .memberTypes = {
                arrayType
            }
        }),
        .addressSpace = Backend::IL::AddressSpace::Constant
    });

    // Id allocations
    descriptorConstantId = table.scan.header.bound++;

    // SpvIds
    SpvId arrayTypeId = table.typeConstantVariable.typeMap.GetSpvTypeId(arrayType);
    SpvId int4DataTypeStructId = table.typeConstantVariable.typeMap.GetSpvTypeId(int4DataTypeStructPtr->pointee);
    SpvId int4DataTypeStructPtrId = table.typeConstantVariable.typeMap.GetSpvTypeId(int4DataTypeStructPtr);

    // Block annotation
    SpvInstruction &pcBlock = table.annotation.block->stream.Allocate(SpvOpDecorate, 3);
    pcBlock[1] = int4DataTypeStructId;
    pcBlock[2] = SpvDecorationBlock;

    // Offset annotation
    SpvInstruction &pcBlockMember = table.annotation.block->stream.Allocate(SpvOpMemberDecorate, 5);
    pcBlockMember[1] = int4DataTypeStructId;
    pcBlockMember[2] = 0;
    pcBlockMember[3] = SpvDecorationOffset;
    pcBlockMember[4] = 0;

    // Array stride annotation
    SpvInstruction &stride = table.annotation.block->stream.Allocate(SpvOpDecorate, 4);
    stride[1] = arrayTypeId;
    stride[2] = SpvDecorationArrayStride;
    stride[3] = 16;

    // Descriptor data
    SpvInstruction &spvCounterVar = table.typeConstantVariable.block->stream.Allocate(SpvOpVariable, 4);
    spvCounterVar[1] = int4DataTypeStructPtrId;
    spvCounterVar[2] = descriptorConstantId;
    spvCounterVar[3] = SpvStorageClassUniform;

    // Descriptor set
    SpvInstruction &spvCounterSet = table.annotation.block->stream.Allocate(SpvOpDecorate, 4);
    spvCounterSet[1] = descriptorConstantId;
    spvCounterSet[2] = SpvDecorationDescriptorSet;
    spvCounterSet[3] = job.instrumentationKey.pipelineLayoutUserSlots;

    // Binding
    SpvInstruction &spvCounterBinding = table.annotation.block->stream.Allocate(SpvOpDecorate, 4);
    spvCounterBinding[1] = descriptorConstantId;
    spvCounterBinding[2] = SpvDecorationBinding;
    spvCounterBinding[3] = job.bindingInfo.descriptorDataDescriptorOffset;
}

IL::ID SpvUtilShaderDescriptorConstantData::GetDescriptorOffset(SpvStream& stream, uint32_t index) {
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();

    // Id allocations
    IL::ID result = table.scan.header.bound++;

    // UInt32
    Backend::IL::IntType typeInt;
    typeInt.bitWidth = 32;
    typeInt.signedness = false;
    const Backend::IL::Type *uintType = ilTypeMap.FindTypeOrAdd(typeInt);

    // UInt32*
    const Backend::IL::Type *uintTypePtr = ilTypeMap.FindTypeOrAdd(Backend::IL::PointerType {
        .pointee = uintType,
        .addressSpace = Backend::IL::AddressSpace::Constant
    });

    // Constant identifiers
    uint32_t zeroUintId = table.scan.header.bound++;
    uint32_t rowUintId = table.scan.header.bound++;
    uint32_t columnUintId = table.scan.header.bound++;

    // SpvIds
    uint32_t uintTypeId = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
    uint32_t uintPtrTypeId = table.typeConstantVariable.typeMap.GetSpvTypeId(uintTypePtr);

    // 0
    SpvInstruction &spvZero = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
    spvZero[1] = uintTypeId;
    spvZero[2] = zeroUintId;
    spvZero[3] = 0;

    // Row
    SpvInstruction &spvRow = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
    spvRow[1] = uintTypeId;
    spvRow[2] = rowUintId;
    spvRow[3] = index / 4;

    // Column
    SpvInstruction &spvColumn = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
    spvColumn[1] = uintTypeId;
    spvColumn[2] = columnUintId;
    spvColumn[3] = index % 4;

    // Address identifier
    uint32_t addressId = table.scan.header.bound++;

    // Get address to element
    SpvInstruction& spvAddress = stream.Allocate(SpvOpAccessChain, 7);
    spvAddress[1] = uintPtrTypeId;
    spvAddress[2] = addressId;
    spvAddress[3] = descriptorConstantId;
    spvAddress[4] = zeroUintId;
    spvAddress[5] = rowUintId;
    spvAddress[6] = columnUintId;

    // Load element
    SpvInstruction& spvLoad = stream.Allocate(SpvOpLoad, 4);
    spvLoad[1] = uintTypeId;
    spvLoad[2] = result;
    spvLoad[3] = addressId;

    // OK
    return result;
}

void SpvUtilShaderDescriptorConstantData::CopyTo(SpvPhysicalBlockTable &remote, SpvUtilShaderDescriptorConstantData &out) {
    out.descriptorConstantId = descriptorConstantId;
}
