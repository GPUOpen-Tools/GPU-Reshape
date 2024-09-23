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

#include <Features/Loop/SignalShaderProgram.h>

// Backend
#include <Backend/IL/ProgramCommon.h>
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/IL/Emitters/Emitter.h>

// Common
#include <Common/Registry.h>

SignalShaderProgram::SignalShaderProgram(ShaderDataID terminationBufferID) : terminationBufferID(terminationBufferID) {

}

bool SignalShaderProgram::Install() {
    // Shader data host
    shaderDataHost = registry->Get<IShaderDataHost>();

    // Create event data
    signalEventID = shaderDataHost->CreateEventData(ShaderDataEventInfo{});

    // OK
    return true;
}

void SignalShaderProgram::Inject(IL::Program &program) {
    // Must have termination block
    IL::BasicBlock* basicBlock = IL::GetTerminationBlock(program);
    if (!basicBlock) {
        return;
    }

    // Get the input data
    IL::ID terminationBufferDataID = program.GetShaderDataMap().Get(terminationBufferID)->id;
    IL::ID signalDataID = program.GetShaderDataMap().Get(signalEventID)->id;

    // Append prior terminator
    IL::Emitter<> emitter(program, *basicBlock, basicBlock->GetTerminator());

    // Write the signal
    emitter.AtomicOr(emitter.AddressOf(terminationBufferDataID, signalDataID), emitter.UInt32(1u));
}
