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

#include <Features/Debug/LooseAcquisitionProgram.h>
#include <Features/Debug/WatchpointHeader.h>

// Backend
#include <Backend/IL/ProgramCommon.h>
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/IL/Emitters/Emitter.h>
#include <Backend/IL/ShaderBufferStruct.h>
#include <Backend/IL/ShaderStruct.h>
#include <Backend/IL/Metadata/KernelMetadata.h>

// Schemas
#include <Schemas/Features/Debug.h>

// Common
#include <Common/Registry.h>

LooseAcquisitionProgram::LooseAcquisitionProgram(ShaderDataID streamBufferID, ShaderExportID exportID) : streamBufferID(streamBufferID), exportID(exportID) {
    
}

bool LooseAcquisitionProgram::Install() {
    // Shader data host
    shaderDataHost = registry->Get<IShaderDataHost>();

    // Create patch data
    dataID = shaderDataHost->CreateDescriptorData(ShaderDataDescriptorInfo::FromStruct<WatchpointLooseAcquisitionData>());

    // OK
    return true;
}

void LooseAcquisitionProgram::Inject(IL::Program &program) {
    // Get entry point
    IL::Function* entryPoint = program.GetEntryPoint();
    
    // Must have termination block
    IL::BasicBlock* entryBlock = Backend::IL::GetTerminationBlock(program);
    if (!entryBlock) {
        return;
    }

    // Launch in shared configuration
    program.GetMetadataMap().AddMetadata(entryPoint->GetID(), IL::KernelWorkgroupSizeMetadata {
        .threadsX = 1,
        .threadsY = 1,
        .threadsZ = 1
    });
    
    IL::BasicBlock* bodyBlock = entryPoint->GetBasicBlocks().AllocBlock();
    IL::BasicBlock* exitBlock = entryPoint->GetBasicBlocks().AllocBlock();

    // Get the data ids
    IL::ID streamDataID = program.GetShaderDataMap().Get(streamBufferID)->id;
    
    // Get shader data
    IL::ShaderStruct<WatchpointLooseAcquisitionData> acquisitionData = program.GetShaderDataMap().Get(dataID)->id;
    
    // Split the entry point for early out
    entryBlock->Split(exitBlock, entryBlock->GetTerminator());

    // Watchpoint header
    IL::ShaderBufferStruct<WatchpointHeader> header;

    IL::Emitter<> entryEmitter(program, *entryBlock);
    {
        // Get the header
        header = IL::ShaderBufferStruct<WatchpointHeader>(streamDataID, acquisitionData.Get<&WatchpointLooseAcquisitionData::allocationDWordOffset>(entryEmitter));

        // Was this produced?
        IL::ID hasProducer = entryEmitter.NotEqual(
            header.Get<&WatchpointHeader::dynamicCounter>(entryEmitter),
            entryEmitter.UInt32(0)
        );
        
        // Only for valid hashes (not sync-safe, but, better than nothing)
        hasProducer = entryEmitter.And(
            hasProducer,
            entryEmitter.NotEqual(
                header.Get<&WatchpointHeader::shaderInstrumentationHash32>(entryEmitter),
                entryEmitter.UInt32(0)
            )
        );
        
        // Skip locked (not thread safe, just to reduce spam)
        hasProducer = entryEmitter.And(
            hasProducer,
            entryEmitter.NotEqual(
                header.Get<&WatchpointHeader::shaderInstrumentationHash32>(entryEmitter),
                entryEmitter.UInt32(kWatchpointInstrumentationHashLocked)
            )
        );
        
        // If acquired, branch out
        entryEmitter.BranchConditional(
            hasProducer,
            bodyBlock,
            exitBlock,
            IL::ControlFlow::Selection(exitBlock)
        );
    }
    
    IL::Emitter<> bodyEmitter(program, *bodyBlock);
    {
        // Pseudo lock the watchpoint by re-acquiring the hash
        IL::ID previousHash = header.AtomicExchange<&WatchpointHeader::shaderInstrumentationHash32>(bodyEmitter, bodyEmitter.UInt32(kWatchpointInstrumentationHashLocked));
        
        // Send acquisition event
        WatchpointAcquisitionMessage::ShaderExport msg;
        msg.chunks |= WatchpointAcquisitionMessage::Chunk::ExtraData | WatchpointAcquisitionMessage::Chunk::LooseCounter;
        msg.uid = acquisitionData.Get<&WatchpointLooseAcquisitionData::watchpointUid>(bodyEmitter);
        msg.magic = bodyEmitter.UInt32(42);
        msg.extraData.instrumentationHash32 = previousHash;
        msg.looseCounter.streamedDynamicCounter = header.Get<&WatchpointHeader::dynamicCounter>(bodyEmitter);
        bodyEmitter.Export(exportID, msg);
        
        // Fin!
        bodyEmitter.Branch(exitBlock);
    }
}
