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

#include <Features/Concurrency/Feature.h>
#include <Features/Descriptor/Feature.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/Emitters/ResourceTokenEmitter.h>
#include <Backend/CommandContext.h>

// Generated schema
#include <Schemas/Features/Concurrency.h>
#include <Schemas/Instrumentation.h>
#include <Schemas/InstrumentationCommon.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStreamCommon.h>

// Common
#include <Common/Registry.h>

bool ConcurrencyFeature::Install() {
    // Must have the export host
    auto exportHost = registry->Get<IShaderExportHost>();
    if (!exportHost) {
        return false;
    }

    // Allocate the shared export
    exportID = exportHost->Allocate<ResourceRaceConditionMessage>();

    // Optional SGUID host
    sguidHost = registry->Get<IShaderSGUIDHost>();

    // Shader data host
    shaderDataHost = registry->Get<IShaderDataHost>();

    // Allocate lock buffer
    //   ? Each respective PUID takes one lock integer, representing the current event id
    lockBufferID = shaderDataHost->CreateBuffer(ShaderDataBufferInfo {
        .elementCount = 1u << Backend::IL::kResourceTokenPUIDBitCount,
        .format = Backend::IL::Format::R32UInt
    });

    // Allocate event data
    //   ? Lightweight event data
    eventID = shaderDataHost->CreateEventData(ShaderDataEventInfo { });

    // OK
    return true;
}

FeatureHookTable ConcurrencyFeature::GetHookTable() {
    FeatureHookTable table{};
    table.dispatch = BindDelegate(this, ConcurrencyFeature::OnDispatch);
    table.drawInstanced = BindDelegate(this, ConcurrencyFeature::OnDrawInstanced);
    table.drawIndexedInstanced = BindDelegate(this, ConcurrencyFeature::OnDrawIndexedInstanced);
    return table;
}

void ConcurrencyFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void ConcurrencyFeature::CollectMessages(IMessageStorage *storage) {
    storage->AddStreamAndSwap(stream);
}

void ConcurrencyFeature::Inject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Options
    const SetInstrumentationConfigMessage config = CollapseOrDefault<SetInstrumentationConfigMessage>(specialization);
    
    // Get the data ids
    IL::ID lockBufferDataID = program.GetShaderDataMap().Get(lockBufferID)->id;
    IL::ID eventDataID = program.GetShaderDataMap().Get(eventID)->id;

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        // Is write operation?
        bool isWrite = false;

        // Instruction of interest?
        IL::ID resource;
        switch (it->opCode) {
            default:
                return it;
            case IL::OpCode::LoadBuffer: {
                resource = it->As<IL::LoadBufferInstruction>()->buffer;
                break;
            }
            case IL::OpCode::StoreBuffer: {
                resource = it->As<IL::StoreBufferInstruction>()->buffer;
                isWrite = true;
                break;
            }
            case IL::OpCode::LoadBufferRaw: {
                resource = it->As<IL::LoadBufferRawInstruction>()->buffer;
                break;
            }
            case IL::OpCode::StoreBufferRaw: {
                resource = it->As<IL::StoreBufferRawInstruction>()->buffer;
                isWrite = true;
                break;
            }
            case IL::OpCode::StoreTexture: {
                resource = it->As<IL::StoreTextureInstruction>()->texture;
                isWrite = true;
                break;
            }
            case IL::OpCode::LoadTexture: {
                resource = it->As<IL::LoadTextureInstruction>()->texture;

                // Get type
                auto type = program.GetTypeMap().GetType(resource)->As<Backend::IL::TextureType>();

                // Sub-pass inputs are not validated
                if (type->dimension == Backend::IL::TextureDimension::SubPass) {
                    return it;
                }
                break;
            }
            case IL::OpCode::SampleTexture: {
                resource = it->As<IL::SampleTextureInstruction>()->texture;
                break;
            }
        }

        // Bind the SGUID
        ShaderSGUID sguid = sguidHost ? sguidHost->Bind(program, it) : InvalidShaderSGUID;

        // Allocate blocks
        IL::BasicBlock* resumeBlock = context.function.GetBasicBlocks().AllocBlock();
        IL::BasicBlock* oobBlock    = context.function.GetBasicBlocks().AllocBlock();

        // Split this basic block, move all instructions post and including the instrumented instruction to resume
        // ! iterator invalidated
        auto instr = context.basicBlock.Split(resumeBlock, it);

        // Perform instrumentation check
        IL::Emitter<> pre(program, context.basicBlock);

        // Get global id of resource
        IL::ResourceTokenEmitter token(pre, resource);
        IL::ID puid = token.GetPUID();

        // Load buffer
        IL::ID bufferValue = pre.Load(lockBufferDataID);

        // Default unlocked value
        IL::ID unlockedConstant = pre.UInt32(0);

        // Get previous lock
        IL::ID previousLock;
        if (isWrite) {
            // Compare exchange at PUID with the event
            // ! Single producer
            previousLock = pre.AtomicCompareExchange(pre.AddressOf(lockBufferDataID, puid), unlockedConstant, eventDataID);
        } else {
            // Read the current lock at PUID
            // ! Multiple consumers
            previousLock = pre.Extract(pre.LoadBuffer(bufferValue, puid), program.GetConstants().UInt(0)->id);
        }

        // If either unacquired or by the current event
        IL::ID cond = pre.And(
            pre.NotEqual(previousLock, pre.UInt32(0)),
            pre.NotEqual(previousLock, eventDataID)
        );

        // If so, branch to failure, otherwise resume
        pre.BranchConditional(cond, oobBlock, resumeBlock, IL::ControlFlow::Selection(resumeBlock));

        // Out of bounds block
        IL::Emitter<> oob(program, *oobBlock);
        oob.AddBlockFlag(BasicBlockFlag::NoInstrumentation);

        // Export the message
        ResourceRaceConditionMessage::ShaderExport msg;
        msg.sguid = oob.UInt32(sguid);
        msg.LUID = eventDataID;

        // Detailed instrumentation?
        if (config.detail) {
            msg.chunks |= ResourceRaceConditionMessage::Chunk::Detail;
            msg.detail.token = token.GetPackedToken();
        }
        
        // Export the message
        oob.Export(exportID, msg);

        // Branch back
        oob.Branch(resumeBlock);

        // Reads have no lock
        if (!isWrite) {
            return instr;
        }

        // Writes release lock after IOI
        IL::Emitter<> resumeEmitter(program, *resumeBlock, ++resumeBlock->begin());
        resumeEmitter.StoreBuffer(bufferValue, puid, unlockedConstant);

        // Resume after unlock
        return resumeEmitter.GetIterator();
    });
}

FeatureInfo ConcurrencyFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Concurrency";
    info.description = "Instrumentation and validation of race conditions across events or queues";

    // Resource bounds requires valid descriptor data, for proper safe-guarding add the descriptor feature as a dependency.
    // This ensures that during instrumentation, we are operating on the already validated, and potentially safe-guarded, descriptor data.
    info.dependencies.push_back(DescriptorFeature::kID);

    // OK
    return info;
}

void ConcurrencyFeature::OnDrawInstanced(CommandContext *context, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    // Assign next event UID, small chance of collision if any event lingers past UINT32_MAX
    context->eventStack.Set(eventID, eventCounter++);
}

void ConcurrencyFeature::OnDrawIndexedInstanced(CommandContext *context, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    // Assign next event UID, small chance of collision if any event lingers past UINT32_MAX
    context->eventStack.Set(eventID, eventCounter++);
}

void ConcurrencyFeature::OnDispatch(CommandContext *context, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) {
    // Assign next event UID, small chance of collision if any event lingers past UINT32_MAX
    context->eventStack.Set(eventID, eventCounter++);
}
