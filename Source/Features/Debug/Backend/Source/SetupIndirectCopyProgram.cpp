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

#include <Features/Debug/SetupIndirectCopyProgram.h>
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

SetupIndirectCopyProgram::SetupIndirectCopyProgram(ShaderDataID streamBufferID, ShaderExportID exportID) : streamBufferID(streamBufferID), exportID(exportID) {
    
}

bool SetupIndirectCopyProgram::Install() {
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

    // Create the host binding
    hostDataID = shaderDataHost->CreateBufferBinding(programID, ShaderDataBufferBindingInfo{
        .isWritable = true,
        .format = Backend::IL::Format::R32UInt
    });

    // OK
    return true;
}

void SetupIndirectCopyProgram::Inject(IL::Program &program) {
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
        IL::ID hostDataBuffer = program.GetShaderDataMap().Get(hostDataID)->id;
        
        // Copy over the header separately
        for (uint32_t i = 0; i < WatchpointHeaderDWordCount; i++) {
            IL::ID value = actEmitter.Extract(
                actEmitter.LoadBuffer(
                    actEmitter.Load(streamDataID), 
                    actEmitter.Add(
                        acquisitionData.Get<&WatchpointCopyData::allocationDWordOffset>(actEmitter),
                        actEmitter.UInt32(i)
                    )
                ),
                actEmitter.UInt32(0)
            );
            
            actEmitter.StoreBuffer(
                actEmitter.Load(hostDataBuffer),
                actEmitter.UInt32(i),
                value
            );
        }
        
        // Total number of dwords produced
        IL::ID streamSizeEvent = header.Get<&WatchpointHeader::dwordStreamCount>(actEmitter);
        
        // Loose dwords produced
        IL::ID streamSizeLoose = actEmitter.Mul(
            header.Get<&WatchpointHeader::dynamicCounter>(actEmitter),
            actEmitter.Add(
                actEmitter.UInt32(WatchpointLooseHeaderDWordCount),
                header.Get<&WatchpointHeader::payloadDataDWordStride>(actEmitter)
            )
        );
        
        // Select actual copy size
        IL::ID streamSize = actEmitter.Select(
            actEmitter.Equal(
                header.Get<&WatchpointHeader::dataOrder>(actEmitter),
                actEmitter.UInt32(static_cast<uint32_t>(WatchpointDataOrder::Loose))
            ),
            streamSizeLoose,
            streamSizeEvent
        );
        
        // Number of groups needed
        IL::ID dispatchCount = actEmitter.Div(
            actEmitter.Add(streamSize, actEmitter.UInt32(63)),
            actEmitter.UInt32(64)
        );
        
        // Write X, 1, 1
        header.Set<&WatchpointHeader::copyDispatchParams>(actEmitter, dispatchCount, 0);
        header.Set<&WatchpointHeader::copyDispatchParams>(actEmitter, actEmitter.UInt32(1), 1);
        header.Set<&WatchpointHeader::copyDispatchParams>(actEmitter, actEmitter.UInt32(1), 2);
        
        // Mark as locked
        header.Set<&WatchpointHeader::copyDispatchLock>(actEmitter, actEmitter.UInt32(1));
        
        actEmitter.Branch(exitBlock);
    }
    
    IL::Emitter<> relEmitter(program, *relBlock);
    {
        // Write 0, 0, 0
        header.Set<&WatchpointHeader::copyDispatchParams>(relEmitter, relEmitter.UInt32(0), 0);
        header.Set<&WatchpointHeader::copyDispatchParams>(relEmitter, relEmitter.UInt32(0), 1);
        header.Set<&WatchpointHeader::copyDispatchParams>(relEmitter, relEmitter.UInt32(0), 2);
        relEmitter.Branch(exitBlock);
    }
}
