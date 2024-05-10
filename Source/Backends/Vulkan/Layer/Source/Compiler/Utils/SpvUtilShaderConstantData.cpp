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

#include <Backends/Vulkan/Compiler/Utils/SpvUtilShaderConstantData.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Compiler/SpvJob.h>

SpvUtilShaderConstantData::SpvUtilShaderConstantData(const Allocators &allocators, IL::Program &program, SpvPhysicalBlockTable &table) :
    allocators(allocators),
    program(program),
    table(table) {

}

void SpvUtilShaderConstantData::CompileRecords(const SpvJob &job) {
    // Capability set
    table.capability.Add(SpvCapabilityImageBuffer);

    // IL type map
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();

    // Number of dwords
    uint32_t dwordCount = 0;

    // Aggregate dword count
    for (const ShaderDataInfo& info : program.GetShaderDataMap()) {
        if (info.type != ShaderDataType::Descriptor) {
            continue;
        }

        dwordCount += info.descriptor.dwordCount;
    }

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
        .count = std::max<uint32_t>(1, (dwordCount + 3) / 4)
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
    constantId = table.scan.header.bound++;

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
    spvCounterVar[2] = constantId;
    spvCounterVar[3] = SpvStorageClassUniform;

    // Descriptor set
    SpvInstruction &spvCounterSet = table.annotation.block->stream.Allocate(SpvOpDecorate, 4);
    spvCounterSet[1] = constantId;
    spvCounterSet[2] = SpvDecorationDescriptorSet;
    spvCounterSet[3] = job.instrumentationKey.pipelineLayoutUserSlots;

    // Binding
    SpvInstruction &spvCounterBinding = table.annotation.block->stream.Allocate(SpvOpDecorate, 4);
    spvCounterBinding[1] = constantId;
    spvCounterBinding[2] = SpvDecorationBinding;
    spvCounterBinding[3] = job.bindingInfo.descriptorDataDescriptorOffset;

    // Add to all entry points
    table.entryPoint.AddInterface(SpvStorageClassUniform, constantId);
}

IL::ID SpvUtilShaderConstantData::GetConstantData(SpvStream& stream, uint32_t index) {
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
    spvAddress[3] = constantId;
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

void SpvUtilShaderConstantData::CopyTo(SpvPhysicalBlockTable &remote, SpvUtilShaderConstantData &out) {
    out.constantId = constantId;
}
