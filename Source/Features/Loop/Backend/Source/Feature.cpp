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

// Generated schema
#include <Schemas/Features/Loop.h>

// Message
#include <Message/IMessageStorage.h>

// Common
#include <Common/Registry.h>
#include <Common/Sink.h>

LoopFeature::LoopFeature() {
    // Start the heart beat thread
    heartBeatThread = std::thread(&LoopFeature::HeartBeatThreadWorker, this);
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
        .format = Backend::IL::Format::R32UInt
    });

    // Allocate allocation data
    terminationAllocationID = shaderDataHost->CreateDescriptorData(ShaderDataDescriptorInfo{
        .dwordCount = 1u
    });

    // OK
    return true;
}

FeatureHookTable LoopFeature::GetHookTable() {
    FeatureHookTable table{};
    table.open = BindDelegate(this, LoopFeature::OnOpen);
    table.submit = BindDelegate(this, LoopFeature::OnSubmit);
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
    // Get the data ids
    IL::ID terminationBufferDataID = program.GetShaderDataMap().Get(terminationBufferID)->id;
    IL::ID terminationAllocationDataID = program.GetShaderDataMap().Get(terminationAllocationID)->id;

    // Get the program capabilities
    const IL::CapabilityTable &capabilityTable = program.GetCapabilityTable();

    // If the program has structured control flow, we can take quite a few liberties in instrumentation
    if (capabilityTable.hasControlFlow) {
        // Visit all instructions
        IL::VisitUserInstructions(program, [&](IL::VisitContext &context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
            IL::BranchControlFlow controlFlow;

            // Must have a continue based, i.e. loop styled, control flow
            if (!Backend::IL::GetControlFlow(it, controlFlow) || controlFlow._continue == IL::InvalidID) {
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
            IL::BasicBlock *postEntry = context.function.GetBasicBlocks().AllocBlock();
            IL::BasicBlock *terminationBlock = context.function.GetBasicBlocks().AllocBlock();

            // The selection merge target, typically this is the post entry (i.e. the split body entry point block),
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

            // Split just prior to loop entry
            entryBlock->Split(postEntry, entryBlock->begin());

            // Emit into pre-guard
            {
                IL::Emitter<> pre(program, *entryBlock);

                // Atomically read the termination data
                IL::ID terminationID = pre.AtomicAnd(pre.AddressOf(terminationBufferDataID, terminationAllocationDataID), pre.UInt32(1u));

                // Early exit if termination was requested
                pre.BranchConditional(
                    pre.Equal(terminationID, pre.UInt32(1u)),
                    terminationBlock,
                    selectionMerge,
                    IL::ControlFlow::Selection(selectionMerge)
                );
            }

            // Emit into termination block
            {
                IL::Emitter<> term(program, *terminationBlock);

                // Export the message
                LoopTerminationMessage::ShaderExport msg;
                msg.sguid = term.UInt32(sguid);
                msg.padding = term.UInt32(0);
                term.Export(exportID, msg);

                // Branch to the merge block
                term.Branch(mergeBlock);
            }

            // Short out merge phis
            for (auto phiIt = mergeBlock->begin(); phiIt != mergeBlock->end() && phiIt->Is<IL::PhiInstruction>();) {
                auto phiInstr = phiIt->As<IL::PhiInstruction>();

                // Number of phi values
                const uint32_t valueCount = phiInstr->values.count;

                // Copy known values
                auto* values = ALLOCA_ARRAY(IL::PhiValue, valueCount + 1u);
                std::memcpy(values, &phiInstr->values[0], sizeof(IL::PhiValue) * valueCount);

                // Create new incoming phi block value, default to null
                values[valueCount] = IL::PhiValue {
                    .value = program.GetConstants().FindConstantOrAdd(program.GetTypeMap().GetType(phiInstr->result), IL::NullConstant {})->id,
                    .branch = terminationBlock->GetID()
                };

                // Replace the phi instruction
                phiIt = std::next(IL::Emitter<IL::Op::Replace>(program, phiIt).Phi(phiInstr->result, valueCount + 1u, values));
            }

            // Iterate next on this instruction
            return postEntry->begin();
        });
    } else {
        // The program does not have structured control flow, therefore we need to perform cfg loop analysis, and pray.
        for (IL::Function *fn: program.GetFunctionList()) {
            // Computer all dominators
            IL::DominatorAnalysis dominatorAnalysis(fn->GetBasicBlocks());
            dominatorAnalysis.Compute();

            // Compute all loops
            IL::LoopAnalysis loopAnalysis(dominatorAnalysis);
            loopAnalysis.Compute();

            // Instrument each loop
            for (const IL::Loop& loop : loopAnalysis.GetView()) {
                // Ignore flagged blocks
                if (loop.header->HasFlag(BasicBlockFlag::NoInstrumentation)) {
                   continue;
                }

                // Allocate blocks
                IL::BasicBlock* postGuardBlock   = fn->GetBasicBlocks().AllocBlock();
                IL::BasicBlock *terminationBlock = fn->GetBasicBlocks().AllocBlock();

                // Bind the SGUID
                ShaderSGUID sguid = sguidHost ? sguidHost->Bind(program, loop.backEdgeBlocks[0]->GetTerminator()) : InvalidShaderSGUID;

                // Split just prior to loop header
                loop.header->Split(postGuardBlock, loop.header->GetTerminator());

                // Emit into pre-guard
                {
                    IL::Emitter<> pre(program, *loop.header);

                    // Atomically read the termination data
                    IL::ID terminationID = pre.AtomicAnd(pre.AddressOf(terminationBufferDataID, terminationAllocationDataID), pre.UInt32(1u));

                    // Early exit if termination was requested
                    pre.BranchConditional(
                        pre.Equal(terminationID, pre.UInt32(1u)),
                        terminationBlock,
                        postGuardBlock,
                        IL::ControlFlow::None()
                    );
                }

                // Emit into termination block
                {
                    IL::Emitter<> term(program, *terminationBlock);

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
    if (submissionAllocationCounter == kMaxTrackedSubmissions) {
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
}

void LoopFeature::OnSubmit(CommandContextHandle contextHandle) {
    std::lock_guard guard(mutex);

    // Validation
    ASSERT(contextStates.contains(contextHandle), "Desynchronized command context states");

    // Mark as pending and record time
    CommandContextState &state = contextStates[contextHandle];
    state.submissionStamp = std::chrono::high_resolution_clock::now();
    state.pending = true;
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

            // Perform staging, ideally with atomic writes
            builder.StageBuffer(terminationBufferID, sizeof(uint32_t) * value.terminationID, sizeof(uint32_t), &stagedValue, StageBufferFlag::Atomic32);

            // Mark as terminated
            value.terminated = true;
        }

        // Any commands?
        if (transferBuffer.Count()) {
            scheduler->Schedule(Queue::Compute, transferBuffer);
        }
    }
}
