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

void DebugEmitter::GetVariables(IL::Program &program, const IL::Instruction *instr, TrivialStackVector<IL::DebugVariable, 4u>& variables) {
    // Get shader state
    ShaderState* shaderState = device->states_Shaders.GetFromUID(program.GetShaderGUID());
    if (!shaderState || !shaderState->module) {
        return;
    }

    // Get traceback
    DXCodeOffsetTraceback traceback = shaderState->module->GetCodeOffsetTraceback(instr->source.codeOffset);
    
    // Try to get the function
    IL::Function *fn = program.GetFunctionList().GetFunction(traceback.functionID);
    if (!fn) {
        return;
    }

    // Get the dwarf info for the code offset
    DXDwarfInfo info = shaderState->module->GetDebug()->GetDwarfInfo(program.GetTypeMap(), fn, instr->source.codeOffset);

    // No values? No reconstruction
    if (info.variables.empty()) {
        return;
    }
    
    // Copy over variables
    for (uint64_t i = 0; i < info.variables.size(); ++i) {
        const DXDwarfVariableValue& source = info.variables[i];
         
        // Target type
        const Backend::IL::Type *type = source.type;
        if (!type) {
            continue;
        }
        
        // Create info
        IL::DebugVariable& dest = variables.Add();
        dest.name = source.name;
        dest.handle = static_cast<uint32_t>(i);

        // TODO: Bit extraction?
        ASSERT(source.values[0].bitWise.bitStart % 8 == 0, "Non-byte aligned");

        // For now, report the first decomposed type at location
        dest.type = Backend::IL::GetStructuredTypeAtOffset(
            type,
            source.values[0].bitWise.bitStart / 8
        );
    }
}

IL::ID DebugEmitter::ReconstructValue(IL::Emitter<> &emitter, uint32_t handle, const IL::Instruction *instr) {
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
    DXDwarfVariableValue& variableValue = info.variables[handle];

    // TODO: Bit extraction?
    ASSERT(variableValue.values[0].bitWise.bitStart % 8 == 0, "Non-byte aligned");

    // For now, report the first decomposed type at location
    const Backend::IL::Type *type = Backend::IL::GetStructuredTypeAtOffset(
        variableValue.type,
        variableValue.values[0].bitWise.bitStart / 8
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
        if (valueTraceback.instructionID == IL::InvalidID) {
            continue;
        }

        // Get instruction
        IL::InstructionRef valueInstr = program.GetIdentifierMap().Get(valueTraceback.instructionID);

        // Append value
#if 0
        IL::Debug::PrettyPrintConsole(program, valueInstr);
#endif // 0
        values.Add(valueInstr->result);
    }

    // May have failed
    if (!values.Size()) {
        return IL::InvalidID;
    }

    // Singular?
    if (values.Size() == 1) {
        return values[0];
    }

    // Construct from the splatted value
    return emitter.ConstructPtr(type, values.Data(), static_cast<uint32_t>(values.Size()));
}
