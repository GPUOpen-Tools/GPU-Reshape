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

#include <Backends/DX12/IL/DebugEmitter.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/States/ShaderState.h>
#include <Backends/DX12/Compiler/IDXDebugModule.h>
#include <Backends/DX12/Compiler/IDXModule.h>

// Backend
#include <Backend/IL/PrettyPrint.h>
#include <Backend/IL/TypeSize.h>

DebugEmitter::DebugEmitter(DeviceState *device) : device(device) {
    
}

const Backend::IL::Type * DebugEmitter::ReconstructValueType(IL::Program &program, const IL::Instruction *instr) {
    // Get shader state
    ShaderState* shaderState = device->states_Shaders.GetFromUID(program.GetShaderGUID());
    if (!shaderState || !shaderState->module) {
        return nullptr;
    }

    // Get traceback
    DXCodeOffsetTraceback traceback = shaderState->module->GetCodeOffsetTraceback(instr->source.codeOffset);
    
    // Try to get the function
    IL::Function *fn = program.GetFunctionList().GetFunction(traceback.functionID);
    if (!fn) {
        return nullptr;
    }

    // Get the dwarf info for the code offset
    DXDwarfInfo info = shaderState->module->GetDebug()->GetDwarfInfo(program.GetTypeMap(), fn, instr->source.codeOffset);

    // No values? No reconstruction
    if (info.variables.empty()) {
        return nullptr;
    }

    // Target type
    const Backend::IL::Type *type = info.variables[0].type;

    // TODO: Bit extraction?
    ASSERT(info.variables[0].values[0].bitWise.bitStart % 8 == 0, "Non-byte aligned");

    // For now, report the first decomposed type at location
    return Backend::IL::GetStructuredTypeAtOffset(
        type,
        info.variables[0].values[0].bitWise.bitStart / 8
    );
}

IL::ID DebugEmitter::ReconstructValue(IL::Emitter<> &emitter, const IL::Instruction *instr) {
    IL::Program &program = *emitter.GetProgram();
    
    // Get shader state
    ShaderState* shaderState = device->states_Shaders.GetFromUID(program.GetShaderGUID());
    if (!shaderState || !shaderState->module) {
        return IL::InvalidID;
    }

    // Get traceback
    DXCodeOffsetTraceback traceback = shaderState->module->GetCodeOffsetTraceback(instr->source.codeOffset);
    
    // Try to get the function
    IL::Function *fn = program.GetFunctionList().GetFunction(traceback.functionID);
    if (!fn) {
        return IL::InvalidID;
    }

    // Get the dwarf info for the code offset
    DXDwarfInfo info = shaderState->module->GetDebug()->GetDwarfInfo(program.GetTypeMap(), fn, instr->source.codeOffset);

    // No values? No reconstruction
    if (info.variables.empty()) {
        return IL::InvalidID;
    }

    // Assumes the first one
    DXDwarfVariableValue& variableValue = info.variables[0];

    // TODO: Bit extraction?
    ASSERT(info.variables[0].values[0].bitWise.bitStart % 8 == 0, "Non-byte aligned");

    // For now, report the first decomposed type at location
    const Backend::IL::Type *type = Backend::IL::GetStructuredTypeAtOffset(
        variableValue.type,
        info.variables[0].values[0].bitWise.bitStart / 8
    );

    // All values
    TrivialStackVector<IL::ID, 16> values;

    // Resolve all the values
    for (const DXDwarfValue& value : variableValue.values) {
        if (value.codeOffset == IL::InvalidID) {
            continue;
        }

        // Get the traceback from the value (debug module -> canonical)
        DXCodeOffsetTraceback valueTraceback = shaderState->module->GetCodeOffsetTraceback(value.codeOffset);

        // Get the block
        IL::BasicBlock *block = fn->GetBasicBlocks().GetBlock(valueTraceback.basicBlockID);
        ASSERT(block, "Failed to associate to canonical module");

        // Instruction indices are linear, advance
        IL::BasicBlock::Iterator instrIt = block->begin();
        std::advance(instrIt, valueTraceback.instructionIndex);

        // Append value
        const IL::Instruction *valueInstr = instrIt.Get();
#if 0
        IL::Debug::PrettyPrintConsole(program, valueInstr);
#endif // 0
        values.Add(valueInstr->result);
    }

    // Singular?
    if (values.Size() == 1) {
        return values[0];
    }

    // Construct from the splatted value
    return emitter.ConstructPtr(type, values.Data(), static_cast<uint32_t>(values.Size()));
}
