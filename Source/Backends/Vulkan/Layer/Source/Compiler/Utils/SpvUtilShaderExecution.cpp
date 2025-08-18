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

#include <Backends/Vulkan/Compiler/Utils/SpvUtilShaderExecution.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Compiler/SpvJob.h>
#include <Backends/Vulkan/Resource/DescriptorData.h>

// Backend
#include <Backend/IL/ResourceTokenMetadataField.h>
#include <Backend/IL/ResourceTokenPacking.h>
#include <Backend/IL/ID.h>

// Common
#include <Common/Containers/TrivialStackVector.h>

SpvUtilShaderExecution::SpvUtilShaderExecution(const Allocators &allocators, IL::Program &program, SpvPhysicalBlockTable &table) :
    allocators(allocators),
    program(program),
    table(table) {
}

void SpvUtilShaderExecution::CompileRecords(const SpvJob &job) {
    
}

void SpvUtilShaderExecution::GetInfo(const SpvJob& job, SpvStream &stream, IL::ID result) {
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();

    // UInt32
    const Backend::IL::Type *uintType = ilTypeMap.FindTypeOrAdd(Backend::IL::IntType {
        .bitWidth = 32,
        .signedness = false
    });

    const Backend::IL::StructType *exeecutionType = ilTypeMap.GetExecutionInfo();
    
    // Constant identifiers
    uint32_t zeroUintId = table.scan.header.bound++;

    // SpvIds
    uint32_t uintTypeId = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
    uint32_t executionTypeId = table.typeConstantVariable.typeMap.GetSpvTypeId(exeecutionType);

    // 0
    SpvInstruction &spvZero = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
    spvZero[1] = uintTypeId;
    spvZero[2] = zeroUintId;
    spvZero[3] = 0;

    // Get the dynamic control offset
    uint32_t executionControlDWordOffset = table.shaderDescriptorConstantData.GetDescriptorData(stream, zeroUintId);

    // All metadata
    TrivialStackVector<SpvId, kExecutionInfoDWordCount> dwordMap;

    // Load all dwords
    for (uint32_t i = 0; i < kExecutionInfoDWordCount; i++) {
        uint32_t fieldOffset = table.scan.header.bound++;

        // Constant identifiers
        uint32_t offsetId = table.scan.header.bound++;
        
        // i
        SpvInstruction &spvZero = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
        spvZero[1] = uintTypeId;
        spvZero[2] = offsetId;
        spvZero[3] = i;
        
        // Base + Offset
        SpvInstruction& spv = stream.Allocate(SpvOpIAdd, 5);
        spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
        spv[2] = fieldOffset;
        spv[3] = executionControlDWordOffset;
        spv[4] = offsetId;
        
        dwordMap.Add(table.shaderDescriptorConstantData.GetDescriptorData(stream, fieldOffset));
    }

    // Create a composite representing all these dwords
    SpvInstruction& spvConstruct = stream.Allocate(SpvOpCompositeConstruct, 3 + kExecutionInfoDWordCount);
    spvConstruct[1] = executionTypeId;
    spvConstruct[2] = result;

    // Fill dwords
    for (uint32_t i = 0; i < kExecutionInfoDWordCount; i++) {
        spvConstruct[3 + i] = dwordMap[i];
    }
}

void SpvUtilShaderExecution::CopyTo(SpvPhysicalBlockTable &remote, SpvUtilShaderExecution &out) {
    
}
