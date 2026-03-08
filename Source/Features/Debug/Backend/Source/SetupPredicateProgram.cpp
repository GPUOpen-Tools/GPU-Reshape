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

#include <Features/Debug/SetupPredicateProgram.h>
#include <Features/Debug/WatchpointHeader.h>

// Backend
#include <Backend/IL/ProgramCommon.h>
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/IL/Emitters/Emitter.h>
#include <Backend/IL/ShaderBufferStruct.h>
#include <Backend/IL/ShaderStruct.h>
#include <Backend/IL/Metadata/KernelMetadata.h>
#include <Backend/ShaderProgram/IShaderProgramHost.h>

// Schemas
#include <Schemas/Features/Debug.h>

// Common
#include <Common/Registry.h>

SetupPredicateProgram::SetupPredicateProgram(ShaderDataID streamBufferID, ShaderExportID exportID) : streamBufferID(streamBufferID), exportID(exportID) {
    
}

bool SetupPredicateProgram::Install() {
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
    dataID = shaderDataHost->CreateDescriptorData(ShaderDataDescriptorInfo::FromStruct<WatchpointCopyData>());

    // OK
    return true;
}

void SetupPredicateProgram::Inject(IL::Program &program) {
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
    
    IL::BasicBlock* acqBlock = entryPoint->GetBasicBlocks().AllocBlock();
    IL::BasicBlock* relBlock = entryPoint->GetBasicBlocks().AllocBlock();
    IL::BasicBlock* exitBlock = entryPoint->GetBasicBlocks().AllocBlock();

    // Get the data ids
    IL::ID streamDataID = program.GetShaderDataMap().Get(streamBufferID)->id;
    
    // Get shader data
    IL::ShaderStruct<WatchpointCopyData> acquisitionData = program.GetShaderDataMap().Get(dataID)->id;
    
    // Split the entry point for early out
    entryBlock->Split(exitBlock, entryBlock->GetTerminator());

    // Watchpoint header
    IL::ShaderBufferStruct<WatchpointHeader> header;

    IL::Emitter<> entryEmitter(program, *entryBlock);
    {
        // Get the header
        header = IL::ShaderBufferStruct<WatchpointHeader>(streamDataID, acquisitionData.Get<&WatchpointCopyData::allocationDWordOffset>(entryEmitter));

        // Was this produced?
        IL::ID hasProducer = entryEmitter.Or(
            entryEmitter.NotEqual(
                header.Get<&WatchpointHeader::acquiredExecutionUID>(entryEmitter),
                entryEmitter.UInt32(0)
            ),
            entryEmitter.NotEqual(
                header.Get<&WatchpointHeader::shaderInstrumentationHash32>(entryEmitter),
                entryEmitter.UInt32(0)
            )
        );
        
        // Has this been copied already?
        hasProducer = entryEmitter.And(
            hasProducer,
            entryEmitter.Equal(
                header.Get<&WatchpointHeader::copyDispatchLock>(entryEmitter),
                entryEmitter.UInt32(0)
            )
        );
        
        // If acquired, branch out
        entryEmitter.BranchConditional(
            hasProducer,
            acqBlock,
            relBlock,
            IL::ControlFlow::Selection(exitBlock)
        );
    }
    
    IL::Emitter<> actEmitter(program, *acqBlock);
    {    
        // Write 1
        header.Set<&WatchpointHeader::predicationLo>(actEmitter, actEmitter.UInt32(1), 0);
        
        // Mark as locked
        header.Set<&WatchpointHeader::copyDispatchLock>(actEmitter, actEmitter.UInt32(1));
        
        actEmitter.Branch(exitBlock);
    }
    
    IL::Emitter<> relEmitter(program, *relBlock);
    {
        // Write 0
        header.Set<&WatchpointHeader::predicationLo>(relEmitter, relEmitter.UInt32(0), 0);
        relEmitter.Branch(exitBlock);
    }
}
