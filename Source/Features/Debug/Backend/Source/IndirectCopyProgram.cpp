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

#include <Features/Debug/IndirectCopyProgram.h>
#include <Features/Debug/WatchpointHeader.h>

// Backend
#include <Backend/IL/ProgramCommon.h>
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/IL/Emitters/Emitter.h>
#include <Backend/IL/ShaderBufferStruct.h>
#include <Backend/IL/ShaderStruct.h>
#include <Backend/IL/Metadata/KernelMetadata.h>
#include <Backend/ShaderProgram/IShaderProgramHost.h>

// Common
#include <Common/Registry.h>

IndirectCopyProgram::IndirectCopyProgram(ShaderDataID streamBufferID, ShaderExportID exportID) : streamBufferID(streamBufferID), exportID(exportID) {
    
}

bool IndirectCopyProgram::Install() {
    // Shader data host
    shaderDataHost = registry->Get<IShaderDataHost>();
    
    // Must have program host
    auto programHost = registry->Get<IShaderProgramHost>();
    if (!programHost) {
        return false;
    }

    // Register validator
    programID = programHost->Register(this);

    // Create patch data
    dataID = shaderDataHost->CreateDescriptorData(ShaderDataDescriptorInfo::FromStruct<WatchpointLooseAcquisitionData>());

    // Create the host binding
    hostDataID = shaderDataHost->CreateBufferBinding(programID, ShaderDataBufferBindingInfo{
        .isWritable = true,
        .format = Backend::IL::Format::R32UInt
    });

    // OK
    return true;
}

void IndirectCopyProgram::Inject(IL::Program &program) {
    // Get entry point
    IL::Function* entryPoint = program.GetEntryPoint();
    
    // Must have termination block
    IL::BasicBlock* entryBlock = Backend::IL::GetTerminationBlock(program);
    if (!entryBlock) {
        return;
    }

    // Insert before terminator
    IL::Emitter<> entryEmitter(program, *entryBlock, entryBlock->GetTerminator());
    
    // Launch in shared configuration
    program.GetMetadataMap().AddMetadata(entryPoint->GetID(), IL::KernelWorkgroupSizeMetadata {
        .threadsX = 64,
        .threadsY = 1,
        .threadsZ = 1
    });
    
    // Get the data ids
    IL::ID streamDataID = program.GetShaderDataMap().Get(streamBufferID)->id;
    IL::ID hostDataBuffer = program.GetShaderDataMap().Get(hostDataID)->id;
    
    // Get shader data
    IL::ShaderStruct<WatchpointCopyData> copyData = program.GetShaderDataMap().Get(dataID)->id;
    
    // Watchpoint header
    IL::ShaderBufferStruct<WatchpointHeader> header = IL::ShaderBufferStruct<WatchpointHeader>(
        streamDataID, 
        copyData.Get<&WatchpointCopyData::allocationDWordOffset>(entryEmitter)
    );
    
    // Get DTID.x
    IL::ID dispatchID = entryEmitter.KernelValue(Backend::IL::KernelValue::DispatchThreadID);
    IL::ID dispatchXID = entryEmitter.Extract(dispatchID, entryEmitter.UInt32(0));
    
    // Offset + DTID.x
    IL::ID payloadOffset = entryEmitter.Add(
        header.Get<&WatchpointHeader::payloadDWordOffset>(entryEmitter),
        dispatchXID
    );
    
    // Load exported dword
    IL::ID value = entryEmitter.Extract(
        entryEmitter.LoadBuffer(
            entryEmitter.Load(streamDataID),
            payloadOffset
        ), 
        entryEmitter.UInt32(0)
    );
    
    // Copy to host visible
    entryEmitter.StoreBuffer(
        entryEmitter.Load(hostDataBuffer),
        entryEmitter.Add(entryEmitter.UInt32(WatchpointHeaderDWordCount), dispatchXID),
        value
    );
}
