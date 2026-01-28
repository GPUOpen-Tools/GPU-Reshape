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

#include <Backends/Vulkan/IL/DebugEmitter.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/ShaderModuleState.h>
#include <Backends/Vulkan/Compiler/SpvDebugMap.h>
#include <Backends/Vulkan/Compiler/SpvModule.h>

DebugEmitter::DebugEmitter(DeviceDispatchTable* table) : table(table) {
    
}

void DebugEmitter::GetVariables(IL::Program &program, const IL::Instruction *instr, TrivialStackVector<IL::DebugVariable, 4u>& variables) {
    // Get shader state
    ShaderModuleState* shaderState = table->states_shaderModule.GetFromUID(program.GetShaderGUID());
    if (!shaderState || !shaderState->spirvModule) {
        return;
    }

    // Must have debug map
    const SpvDebugMap *debugMap = shaderState->spirvModule->GetDebugMap();
    if (!debugMap) {
        return;
    }
    
    // If store, find the delegating variable
    if (auto* storeInstr = instr->Cast<IL::StoreInstruction>()) {
        if (auto it = debugMap->bindingInfos.find(storeInstr->address); it != debugMap->bindingInfos.end()) {
            const SpvDebugVariableInfo &variableInfo = debugMap->variableInfos.at(it->second.debugVariable);
            
            // Create variable
            IL::DebugVariable &dest = variables.Add();
            dest.type = program.GetTypeMap().GetType(variableInfo.typeId);
            dest.name = debugMap->Get(variableInfo.nameId, SpvOpString);
            dest.handle = storeInstr->value;
        }
    }
    
    // Has direct value info?
    auto it = debugMap->instructionValueInfos.find(instr->source.codeOffset);
    if (it != debugMap->instructionValueInfos.end()) {
        for (const InstructionValueInfo &valueInfo: it->second.values) {
            const SpvDebugVariableInfo &variableInfo = debugMap->variableInfos.at(valueInfo.debugVariableId);

            // Create variable
            IL::DebugVariable &dest = variables.Add();
            dest.type = program.GetTypeMap().GetType(variableInfo.typeId);
            dest.name = debugMap->Get(variableInfo.nameId, SpvOpString);
            dest.handle = valueInfo.value;
        }
    }
}

IL::ID DebugEmitter::ReconstructValue(IL::Emitter<> &emitter, uint32_t handle, const IL::Instruction *instr) {
    // Nothing to reconstruct
    return handle;
}
