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

#include <Features/Loop/Feature.h>
#include <Features/Descriptor/Feature.h>
#include <Features/Loop/SignalShaderProgram.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/InstructionCommon.h>
#include <Backend/CommandContext.h>
#include <Backend/IL/Analysis/CFG/DominatorAnalysis.h>
#include <Backend/IL/Analysis/CFG/LoopAnalysis.h>
#include <Backend/ShaderData/ShaderDataDescriptorInfo.h>
#include <Backend/Command/CommandBuilder.h>
#include <Backend/Scheduler/IScheduler.h>
#include <Backend/ShaderProgram/IShaderProgramHost.h>

// Generated schema
#include <Schemas/Features/Loop.h>
#include <Schemas/Features/LoopConfig.h>
#include <Schemas/Instrumentation.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStreamCommon.h>

// Common
#include <Common/Registry.h>
#include <Common/Sink.h>

// Use feature programs instead of staging buffers for CPU-fed signalling
#define USE_SIGNAL_PROGRAM 0

LoopFeature::LoopFeature() {
    /** poof */
}

LoopFeature::~LoopFeature() {
    heartBeatExitFlag = true;

    // Wait for the heart beat
    heartBeatThread.join();
}

bool LoopFeature::Install() {
    // Must have the export host
    auto exportHost = registry->Get<IShaderExportHost>();
    if (!exportHost) {
        return false;
    }

    // Allocate the shared export
    exportID = exportHost->Allocate<LoopTerminationMessage>();

    // Optional SGUID host
    sguidHost = registry->Get<IShaderSGUIDHost>();

    // Shader data host
    shaderDataHost = registry->Get<IShaderDataHost>();

    // Scheduler
    scheduler = registry->Get<IScheduler>();

    // Allocate termination buffer
    terminationBufferID = shaderDataHost->CreateBuffer(ShaderDataBufferInfo{
        .elementCount = kMaxTrackedSubmissions,
        .format = IL::Format::R32UInt
    });

    // Allocate allocation data
    terminationAllocationID = shaderDataHost->CreateDescriptorData(ShaderDataDescriptorInfo{
        .dwordCount = 1u
    });

#if USE_SIGNAL_PROGRAM
    // Must have program host
    auto programHost = registry->Get<IShaderProgramHost>();
    if (!programHost) {
        return false;
    }

    // Create the signal program
    signalShaderProgram = registry->New<SignalShaderProgram>(terminationBufferID);
    if (!signalShaderProgram->Install()) {
        return false;
    }

    // Register signaller
    signalShaderProgramID = programHost->Register(signalShaderProgram);
#endif // USE_SIGNAL_PROGRAM
    
    // OK
    return true;
}

bool LoopFeature::PostInstall() {
    // Start the heart beat thread
    heartBeatThread = std::thread(&LoopFeature::HeartBeatThreadWorker, this);
    
    // OK
    return true;
}

FeatureHookTable LoopFeature::GetHookTable() {
    FeatureHookTable table{};
    table.open = BindDelegate(this, LoopFeature::OnOpen);
    table.postSubmit = BindDelegate(this, LoopFeature::OnPostSubmit);
    table.join = BindDelegate(this, LoopFeature::OnJoin);
    return table;
}

void LoopFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void LoopFeature::CollectMessages(IMessageStorage *storage) {
    storage->AddStreamAndSwap(stream);
}

void LoopFeature::Inject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Options
    const SetLoopInstrumentationConfigMessage config = FindOrDefault(specialization, SetLoopInstrumentationConfigMessage {
        .useIterationLimits = true,
        .iterationLimit = 32'000,
        .atomicIterationInterval = 256
    });

    // Get constant literals
    IL::ID interval      = program.GetConstants().UInt(config.atomicIterationInterval)->id;
    IL::ID maxIterations = program.GetConstants().UInt(config.iterationLimit)->id;
    
    // Get the data ids
    IL::ID terminationBufferDataID     = program.GetShaderDataMap().Get(terminationBufferID)->id;
    IL::ID terminationAllocationDataID = program.GetShaderDataMap().Get(terminationAllocationID)->id;

    // Get the program capabilities
    const IL::CapabilityTable &capabilityTable = program.GetCapabilityTable();

    // Allocate all the counters
    LoopCounterMap functionCounters;
    InjectLoopCounters(program, functionCounters);

    // If the program has structured control flow, we can take quite a few liberties in instrumentation
    if (capabilityTable.hasControlFlow) {
        // Visit all instructions
        IL::VisitUserInstructions(program, [&](IL::VisitContext &context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
            IL::BranchControlFlow controlFlow;

            // Must have a continue based, i.e. loop styled, control flow
            if (!IL::GetControlFlow(it, controlFlow) || controlFlow._continue == IL::InvalidID) {
                return it;
            }

            // All basic blocks
            IL::BasicBlockList& basicBlocks = context.function.GetBasicBlocks();

            // Bind the SGUID
            ShaderSGUID sguid = sguidHost ? sguidHost->Bind(program, it) : InvalidShaderSGUID;

            // Get merge block
            // On termination this is the block we reach
            IL::BasicBlock* mergeBlock = context.function.GetBasicBlocks().GetBlock(controlFlow.merge);
            
            // Allocate blocks
            IL::BasicBlock *postEntry        = context.function.GetBasicBlocks().AllocBlock();
            IL::BasicBlock *terminationBlock = context.function.GetBasicBlocks().AllocBlock();
            IL::BasicBlock* atomicBlock      = context.function.GetBasicBlocks().AllocBlock();
            IL::BasicBlock* atomicMergeBlock = context.function.GetBasicBlocks().AllocBlock();

            // The selection merge target, typically this is the post-entry (i.e. the split body entry point block),
            // however, if the continue block is the body, then we need to split things further.
            IL::BasicBlock* selectionMerge = postEntry;

            // Expected entry point
            IL::BasicBlock* entryBlock = nullptr;

            // Determine entry point
            switch (it->opCode) {
                default: {
                    return it;
                }
                case IL::OpCode::Branch: {
                    auto* instr = it->As<IL::BranchInstruction>();
                    entryBlock = basicBlocks.GetBlock(instr->branch);
                    
                    // If the body is the continue block, we effectively need another block. Selection merges
                    // within a loop construct must merge to another block inside said construct, continue blocks
                    // are not part of this construct.
                    if (entryBlock->GetID() == controlFlow._continue) {
                        selectionMerge = context.function.GetBasicBlocks().AllocBlock();

                        // Branch to the real continue block
                        IL::Emitter<>(program, *selectionMerge).Branch(postEntry);

                        // Replace the original loop branch, re-route the continue block
                        IL::Emitter<IL::Op::Replace>(program, it).Branch(
                            basicBlocks.GetBlock(instr->branch),
                            IL::ControlFlow::Loop(basicBlocks.GetBlock(controlFlow.merge), postEntry)
                        );
                    }
                    break;
                }
                case IL::OpCode::BranchConditional: {
                    auto* instr = it->As<IL::BranchConditionalInstruction>();
                    entryBlock = context.function.GetBasicBlocks().GetBlock(instr->pass != controlFlow.merge ? instr->pass : instr->fail);

                    // If the body is the continue block, we effectively need another block. Selection merges
                    // within a loop construct must merge to another block inside said construct, continue blocks
                    // are not part of this construct.
                    if (entryBlock->GetID() == controlFlow._continue) {
                        selectionMerge = context.function.GetBasicBlocks().AllocBlock();

                        // Branch to the real continue block
                        IL::Emitter<>(program, *selectionMerge).Branch(postEntry);

                        // Replace the original loop branch, re-route the continue block
                        IL::Emitter<IL::Op::Replace>(program, it).BranchConditional(
                            instr->cond,
                            basicBlocks.GetBlock(instr->pass),
                            basicBlocks.GetBlock(instr->fail),
                            IL::ControlFlow::Loop(basicBlocks.GetBlock(controlFlow.merge), postEntry)
                        );
                    }
                    break;
                }
            }

            // Split from the beginning, handles phi splitting
            entryBlock->Split(postEntry, entryBlock->begin());

            // Emit into pre-guard
            {
                IL::Emitter<> pre(program, *entryBlock);

                // Increment local counter
                IL::ID counter = GetAndIncrementCounter(pre, &context.function, functionCounters);

                // Periodic check, I % Interval == 0
                pre.BranchConditional(
                    pre.Equal(pre.Rem(counter, interval), pre.UInt32(0u)),
                    atomicBlock,
                    selectionMerge,
                    IL::ControlFlow::Selection(selectionMerge)
                );

                // Block performing atomic check
                {
                    IL::Emitter<> atomic(program, *atomicBlock);

                    // Atomically read the termination data
                    IL::ID terminationID = atomic.AtomicAnd(atomic.AddressOf(terminationBufferDataID, terminationAllocationDataID), atomic.UInt32(1u));

                    // Check for a termination signal
                    IL::ID terminated = atomic.Equal(terminationID, atomic.UInt32(1u));

                    // Additionally, check for iteration limits
                    if (config.useIterationLimits) {
                        terminated = atomic.Or(terminated, atomic.GreaterThanEqual(counter, maxIterations));
                    }
                    
                    // Early exit if termination was requested
                    atomic.BranchConditional(
                        terminated,
                        terminationBlock,
                        atomicMergeBlock,
                        IL::ControlFlow::Selection(atomicMergeBlock)
                    );
                }

                // Merge block just merges to the other selection construct, makes SCF happy
                IL::Emitter<>(program, *atomicMergeBlock).Branch(selectionMerge);
            }

            // Emit into termination block
            {
                IL::Emitter<> term(program, *terminationBlock);

                // If iteration limits are enabled, broadcast termination to all other instances
                if (config.useIterationLimits) {
                    term.AtomicOr(term.AddressOf(terminationBufferDataID, terminationAllocationDataID), term.UInt32(1u));
                }
                
                // Export the message
                LoopTerminationMessage::ShaderExport msg;
                msg.sguid = term.UInt32(sguid);
                msg.padding = term.UInt32(0);
                term.Export(exportID, msg);

                // Expected function type
                const IL::Type *returnType = context.function.GetFunctionType()->returnType;

                // If there's something to return, assume null
                IL::ID returnValue = IL::InvalidID;
                if (!returnType->Is<IL::VoidType>()) {
                    returnValue = program.GetConstants().FindConstantOrAdd(returnType, IL::NullConstant {})->id;
                }
                
                // Branch to the merge block
                term.Return(returnValue);
            }

            // Iterate next on this instruction
            return postEntry->begin();
        });
    } else {
        // The program does not have structured control flow, therefore we need to perform cfg loop analysis, and pray.
        for (IL::Function *fn: program.GetFunctionList()) {
            // Compute loop analysis
            ComRef loopAnalysis = fn->GetAnalysisMap().FindPassOrCompute<IL::LoopAnalysis>(*fn);

            // Instrument each loop
            for (const IL::Loop& loop : loopAnalysis->GetView()) {
                // Ignore flagged blocks
                if (loop.header->HasFlag(BasicBlockFlag::NoInstrumentation)) {
                   continue;
                }

                // Allocate blocks
                IL::BasicBlock* postGuardBlock   = fn->GetBasicBlocks().AllocBlock();
                IL::BasicBlock *terminationBlock = fn->GetBasicBlocks().AllocBlock();
                IL::BasicBlock* atomicBlock      = fn->GetBasicBlocks().AllocBlock();

                // Bind the SGUID
                ShaderSGUID sguid = sguidHost ? sguidHost->Bind(program, loop.backEdgeBlocks[0]->GetTerminator()) : InvalidShaderSGUID;

                // Split just prior to loop header
                loop.header->Split(postGuardBlock, loop.header->GetTerminator());

                // Emit into pre-guard
                {
                    IL::Emitter<> pre(program, *loop.header);

                    // Increment local counter
                    IL::ID counter = GetAndIncrementCounter(pre, fn, functionCounters);

                    // Early exit if termination was requested
                    pre.BranchConditional(
                        pre.Equal(pre.Rem(counter, interval), pre.UInt32(0u)),
                        atomicBlock,
                        postGuardBlock,
                        IL::ControlFlow::None()
                    );
            
                    // Block performing atomic check
                    {
                        IL::Emitter<> atomic(program, *atomicBlock);

                        // Atomically read the termination data
                        IL::ID terminationID = atomic.AtomicAnd(atomic.AddressOf(terminationBufferDataID, terminationAllocationDataID), atomic.UInt32(1u));

                        // Check for a termination signal
                        IL::ID terminated = atomic.Equal(terminationID, atomic.UInt32(1u));

                        // Additionally, check for iteration limits
                        if (config.useIterationLimits) {
                            terminated = atomic.Or(terminated, atomic.GreaterThanEqual(counter, maxIterations));
                        }
                        
                        // Early exit if termination was requested
                        atomic.BranchConditional(
                            terminated,
                            terminationBlock,
                            postGuardBlock,
                            IL::ControlFlow::None()
                        );
                    }
                }

                // Emit into termination block
                {
                    IL::Emitter<> term(program, *terminationBlock);

                    // If iteration limits are enabled, broadcast termination to all other instances
                    if (config.useIterationLimits) {
                        term.AtomicOr(term.AddressOf(terminationBufferDataID, terminationAllocationDataID), term.UInt32(1u));
                    }

                    // Export the message
                    LoopTerminationMessage::ShaderExport msg;
                    msg.sguid = term.UInt32(sguid);
                    msg.padding = term.UInt32(0);
                    term.Export(exportID, msg);

                    // Exit the kernel entirely
                    term.Return();
                }
            }
        }
    }
}

void LoopFeature::InjectLoopCounters(IL::Program &program, LoopCounterMap &map) {
    for (IL::Function *function : program.GetFunctionList()) {
        IL::BasicBlock* entryPoint = function->GetBasicBlocks().GetEntryPoint();

        // Inject counter allocation at the start of the function
        IL::Emitter emitter(program, *entryPoint, entryPoint->begin());

        // Allocate UInt32
        IL::ID addr = emitter.Alloca(program.GetTypeMap().FindTypeOrAdd(IL::IntType {
            .bitWidth = 32,
            .signedness = false
        }));

        // Default 0
        emitter.Store(addr, program.GetConstants().UInt(0)->id);
        map[function->GetID()] = addr;
    }
}

IL::ID LoopFeature::GetAndIncrementCounter(IL::Emitter<>& emitter, IL::Function *function, LoopCounterMap &map) {
    IL::ID counterAddr = map[function->GetID()];

    // Load current value
    IL::ID counter = emitter.Load(counterAddr);

    // Store +1
    emitter.Store(counterAddr, emitter.Add(counter, emitter.GetProgram()->GetConstants().UInt(1u)->id));
    return counter;
}

FeatureInfo LoopFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Loop";
    info.description = "Instrumentation and validation of infinite loops";
    return info;
}

uint32_t LoopFeature::AllocateTerminationIDNoLock() {
    // Set id
    uint32_t id = submissionAllocationCounter++;

    // Cycle back if needed
    if (submissionAllocationCounter == kMaxTrackedSubmissions - 1u) {
        submissionAllocationCounter = 0;
    }

    // OK
    return id;
}

void LoopFeature::OnOpen(CommandContext *context) {
    std::lock_guard guard(mutex);

    // Create new state
    CommandContextState &state = contextStates[context->handle];
    state.pending = false;
    state.terminated = false;
    state.terminationID = AllocateTerminationIDNoLock();

    // Update the descriptor data
    CommandBuilder builder(context->buffer);
    builder.SetDescriptorData(terminationAllocationID, state.terminationID);

    // Stage cleared value
    uint32_t noSignalValue = 0u;
    builder.StageBuffer(terminationBufferID, sizeof(uint32_t) * state.terminationID, sizeof(uint32_t), &noSignalValue);
}

void LoopFeature::OnPostSubmit(const CommandContextHandle* contextHandles, uint32_t count) {
    std::lock_guard guard(mutex);
    
    for (uint32_t i = 0; i < count; i++) {
        // Validation
        ASSERT(contextStates.contains(contextHandles[i]), "Desynchronized command context states");

        // Mark as pending and record time
        CommandContextState &state = contextStates[contextHandles[i]];
        state.submissionStamp = std::chrono::high_resolution_clock::now();
        state.pending = true;
    }
}

void LoopFeature::OnJoin(CommandContextHandle contextHandle) {
    std::lock_guard guard(mutex);

    // Validation
    ASSERT(contextStates.contains(contextHandle), "Desynchronized command context states");

    // Update state
    CommandContextState &state = contextStates[contextHandle];
    state.pending = false;
}

void LoopFeature::HeartBeatThreadWorker() {
    // Note: The OS doesn't actually guarantee that the thread will be scheduled back in, but, it's likely good enough
    static constexpr uint32_t PulseIntervalMS = 25;

    // TODO: Exactly what is the right magical figure here?
    static constexpr uint32_t DistanceTerminationMS = 750;

    // Worker loop
    while (!heartBeatExitFlag.load()) {
        // Innocent yield
        std::this_thread::sleep_for(std::chrono::milliseconds(PulseIntervalMS));

        // Current time
        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

        // Serial!
        std::lock_guard guard(mutex);

        // Command buffer for stages
        CommandBuffer  transferBuffer;
        CommandBuilder builder(transferBuffer);

        // Check all pending contexts
        for (auto&& [key, value] : contextStates) {
            if (!value.pending || value.terminated) {
                continue;
            }

            // Determine the current distance
            uint64_t distanceMS = std::chrono::duration_cast<std::chrono::milliseconds>(now - value.submissionStamp).count();

            // Within accepted pulse time?
            if (distanceMS < DistanceTerminationMS) {
                continue;
            }

            // Termination staging value
            uint32_t stagedValue = 1u;

#if USE_SIGNAL_PROGRAM
            // Atomically signal
            builder.SetShaderProgram(signalShaderProgramID);
            builder.SetEventData(signalShaderProgram->GetSignalEventID(), value.terminationID);
            builder.Dispatch(1, 1, 1);
            builder.UAVBarrier();
#else // USE_SIGNAL_PROGRAM
            // Perform staging, ideally with atomic writes
            builder.StageBuffer(terminationBufferID, sizeof(uint32_t) * value.terminationID, sizeof(uint32_t), &stagedValue, StageBufferFlag::Atomic32);
#endif // USE_SIGNAL_PROGRAM

            // Mark as terminated
            value.terminated = true;
        }

        // Any commands?
        if (transferBuffer.Count()) {
            scheduler->Schedule(Queue::Compute, transferBuffer, nullptr);
        }
    }
}
