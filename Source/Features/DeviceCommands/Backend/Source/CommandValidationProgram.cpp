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

#include <Features/DeviceCommands/CommandValidationProgram.h>
#include <Features/DeviceCommands/CommandValidationData.h>

// Backend
#include <Backend/IL/ProgramCommon.h>
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/IL/Emitters/Emitter.h>
#include <Backend/IL/Emitters/IDeviceCommandEmitter.h>
#include <Backend/IL/Metadata/KernelMetadata.h>
#include <Backend/IL/PrettyPrint.h>
#include <Backend/IL/ShaderStruct.h>
#include <Backend/ShaderProgram/IShaderProgramHost.h>

// Schemas
#include <Schemas/Features/DeviceCommands.h>

// Common
#include <Common/Containers/TrivialStackVector.h>

CommandValidationProgram::CommandValidationProgram(ShaderExportID exportID) : exportID(exportID) {
    
}

bool CommandValidationProgram::Install() {
    // Shader data host
    shaderDataHost = registry->Get<IShaderDataHost>();
    
    // Must have program host
    auto programHost = registry->Get<IShaderProgramHost>();
    if (!programHost) {
        return false;
    }

    // Register validator
    programID = programHost->Register(this);

    // Create global data
    dataID = shaderDataHost->CreateDescriptorData(ShaderDataDescriptorInfo::FromStruct<CommandValidationData>());

    // Create the signature binding
    signatureDataID = shaderDataHost->CreateBufferBinding(programID, ShaderDataBufferBindingInfo{
        .format = Backend::IL::Format::R32UInt
    });

    // Create the command binding
    sourceCommandID = shaderDataHost->CreateBufferBinding(programID, ShaderDataBufferBindingInfo {
        .format = Backend::IL::Format::R32UInt
    });

    // Create the patched command binding
    destCommandID = shaderDataHost->CreateBufferBinding(programID, ShaderDataBufferBindingInfo {
        .isWritable = true,
        .format = Backend::IL::Format::R32UInt
    });

    // OK
    return true;
}

void CommandValidationProgram::Inject(IL::Program &program) {
    // Get entry point
    IL::Function* entryPoint = program.GetEntryPoint();
    
    // Must have termination block
    IL::BasicBlock* entryBlock = Backend::IL::GetTerminationBlock(program);
    if (!entryBlock) {
        return;
    }

    // Assume 1, 1, 1
    program.GetMetadataMap().AddMetadata(program.GetEntryPoint()->GetID(), IL::KernelWorkgroupSizeMetadata {1, 1, 1});

    // Shared descriptor data
    IL::ShaderStruct<CommandValidationData> data = program.GetShaderDataMap().Get(dataID)->id;

    // Format creator
    ComRef deviceCommandFormat = registry->Get<IL::IDeviceCommandFormat>();

    // Create the blocks
    IL::BasicBlock* loopHeader    = entryPoint->GetBasicBlocks().AllocBlock("L::Header");
    IL::BasicBlock* loopBody      = entryPoint->GetBasicBlocks().AllocBlock("L::Body");
    IL::BasicBlock* continueBlock = entryPoint->GetBasicBlocks().AllocBlock("L::Continue");
    IL::BasicBlock* exitBlock     = entryPoint->GetBasicBlocks().AllocBlock("Exit");

    // Split off into the exit block
    entryBlock->Split(exitBlock, entryBlock->GetTerminator());

    IL::Emitter<> entryEmitter(program, *entryBlock);

    // Create a new format emitter
    ComRef commandEmitter = deviceCommandFormat->CreateEmitter(
        entryEmitter,
        program.GetShaderDataMap().Get(signatureDataID)->id,
        program.GetShaderDataMap().Get(sourceCommandID)->id,
        program.GetShaderDataMap().Get(destCommandID)->id
    );

    // Total number of commands
    IL::ID commandCount = commandEmitter->GetCount();

    // Current command index
    IL::ID commandIndex = entryEmitter.Alloca(program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType {
        .bitWidth = 32,
        .signedness = false
    }));

    // CommandIndex = 0
    entryEmitter.Store(commandIndex, entryEmitter.UInt32(0));

    // Start looping
    entryEmitter.Branch(loopHeader);

    // Loop Header
    {
        IL::Emitter emitter(program, *loopHeader);

        // CommandIndex < Count?
        emitter.BranchConditional(
            emitter.LessThan(emitter.Load(commandIndex), commandCount),
            loopBody, exitBlock,
            IL::ControlFlow::Loop(exitBlock, continueBlock)
        );
    }

    // Loop Body
    {

        IL::BasicBlock* switchMerge = entryPoint->GetBasicBlocks().AllocBlock("S::Merge");
        
        // All command type cases
        TrivialStackVector<IL::SwitchCase, 4u> cases;
        
        // Dispatch Command
        {
            IL::BasicBlock* caseEntryBlock      = entryPoint->GetBasicBlocks().AllocBlock("S::CaseEntry");
            IL::BasicBlock* caseMergeBlock      = entryPoint->GetBasicBlocks().AllocBlock("S::CaseMerge");
            IL::BasicBlock* invalidPayloadBlock = entryPoint->GetBasicBlocks().AllocBlock("S::Default");

            // Case entry block
            {
                IL::Emitter caseEntryEmitter(program, *caseEntryBlock);

                // Load the dispatch payload
                IL::DeviceCommandDispatchPayload payload = commandEmitter->LoadDispatchPayload(caseEntryEmitter, caseEntryEmitter.Load(commandIndex));

                // Get the current limit
                IL::ID limit = data.Get<&CommandValidationData::dispatchGroupLimit>(caseEntryEmitter);
                
                // Validate dispatch group sizes
                IL::ID isUnsafe = caseEntryEmitter.GreaterThanEqual(payload.groupCountX, limit);
                isUnsafe = caseEntryEmitter.Or(isUnsafe, caseEntryEmitter.GreaterThanEqual(payload.groupCountY, limit));
                isUnsafe = caseEntryEmitter.Or(isUnsafe, caseEntryEmitter.GreaterThanEqual(payload.groupCountZ, limit));

                // Handle exporting
                caseEntryEmitter.BranchConditional(
                    isUnsafe,
                    invalidPayloadBlock,
                    caseMergeBlock,
                    IL::ControlFlow::Selection(caseMergeBlock)
                );
            }

            // Case validation error block
            {
                IL::Emitter invalidEmitter(program, *invalidPayloadBlock);

                // Safeguard the invalid payload (x -> 0)
                commandEmitter->StorePayload(invalidEmitter, invalidEmitter.Load(commandIndex), 0, invalidEmitter.UInt32(0));
                
                // Export the message
                DeviceCommandInvalidArgumentMessage::ShaderExport msg;
                msg.type = invalidEmitter.UInt32(0);
                invalidEmitter.Export(exportID, msg);

                // Merge
                invalidEmitter.Branch(caseMergeBlock);
            }

            // Case merge
            {
                IL::Emitter mergeEmitter(program, *caseMergeBlock);
                mergeEmitter.Branch(switchMerge);
            }

            // Add switch case
            cases.Add(IL::SwitchCase {
                .literal = program.GetConstants().UInt(static_cast<uint32_t>(IL::DeviceCommandType::Dispatch))->id,
                .branch = caseEntryBlock->GetID()
            });
        }

        {
            IL::Emitter loopEmitter(program, *loopBody);
        
            // Get the type of the command
            IL::ID type = commandEmitter->LoadType(loopEmitter, loopEmitter.Load(commandIndex));

            // Switch on the type
            loopEmitter.Switch(
                type, switchMerge,
                 static_cast<uint32_t>(cases.Size()), cases.Data(),
                 IL::ControlFlow::Selection(switchMerge)
            );
        }

        // Merge back to the loop continue block
        {
            IL::Emitter mergeEmitter(program, *switchMerge);
            mergeEmitter.Branch(continueBlock);
        }
    }

    // Loop Continue
    {
        IL::Emitter emitter(program, *continueBlock);
        emitter.Store(commandIndex, emitter.Add(emitter.Load(commandIndex), emitter.UInt32(1)));
        emitter.Branch(loopHeader);
    }
}
