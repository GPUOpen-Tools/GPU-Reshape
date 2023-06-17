// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

#include <Features/Descriptor/Feature.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/ResourceTokenEmitter.h>
#include <Backend/IL/ResourceTokenType.h>
#include <Backend/CommandContext.h>
#include <Backend/IL/BasicBlockFlags.h>
#include <Backend/IL/ResourceTokenPacking.h>
#include <Backend/IL/BasicBlockCommon.h>
#include <Backend/IL/Constant.h>

// Generated schema
#include <Schemas/Features/Descriptor.h>
#include <Schemas/Instrumentation.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStreamCommon.h>
#include <Message/MessageStream.h>

// Common
#include <Common/Registry.h>

bool DescriptorFeature::Install() {
    // Must have the export host
    auto exportHost = registry->Get<IShaderExportHost>();
    if (!exportHost) {
        return false;
    }

    // Allocate the shared export
    exportID = exportHost->Allocate<DescriptorMismatchMessage>();

    // Optional SGUID host
    sguidHost = registry->Get<IShaderSGUIDHost>();

    // OK
    return true;
}

FeatureHookTable DescriptorFeature::GetHookTable() {
    FeatureHookTable table{};
    return table;
}

void DescriptorFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void DescriptorFeature::CollectMessages(IMessageStorage *storage) {
    storage->AddStreamAndSwap(stream);
}

IL::BasicBlock::Iterator DescriptorFeature::InjectForResource(IL::Program &program, IL::Function& function, IL::BasicBlock::Iterator it, IL::ID resource, Backend::IL::ResourceTokenType compileTypeLiteral, const SetInstrumentationConfigMessage& config) {
    IL::BasicBlock* basicBlock = it.block;

    // Do we need a merge (phi) for result data?
    const bool needsSafeGuardCFMerge = config.safeGuard && it->result != IL::InvalidID;

    // Resulting type of the instruction (safe-guarding)
    const Backend::IL::Type* resultType;
    if (needsSafeGuardCFMerge) {
        resultType = program.GetTypeMap().GetType(it->result);
    }

    // Bind the SGUID
    ShaderSGUID sguid = sguidHost ? sguidHost->Bind(program, it) : InvalidShaderSGUID;

    // Allocate resume
    IL::BasicBlock *resumeBlock = function.GetBasicBlocks().AllocBlock();

    // Split this basic block, move all instructions post and including the instrumented instruction to resume
    // ! iterator invalidated
    auto instr = basicBlock->Split(resumeBlock, config.safeGuard ? std::next(it) : it);

    // Safeguard identifiers, later merged with phi
    IL::ID safeGuardValue    = it->result;
    IL::ID safeGuardRedirect = IL::InvalidID;
    IL::ID safeGuardZero     = IL::InvalidID;

    // If needed, move offending instruction to a safeguarded block
    IL::BasicBlock* matchBlock;
    if (config.safeGuard) {
        // Allocate match block
        matchBlock = function.GetBasicBlocks().AllocBlock();

        // Move offending instruction to this block
        IL::BasicBlock::Iterator splitIt = basicBlock->Split(matchBlock, it);

        // Redirect the user instruction
        if (needsSafeGuardCFMerge) {
            safeGuardRedirect = program.GetIdentifierMap().AllocID();
            Backend::IL::RedirectResult(program, splitIt, safeGuardRedirect);
        }

        // Branch back to resume
        IL::Emitter<> match(program, *matchBlock);
        match.Branch(resumeBlock);
    }

    // Allocate failure block
    IL::BasicBlock* mismatchBlock = function.GetBasicBlocks().AllocBlock();
    mismatchBlock->AddFlag(BasicBlockFlag::NoInstrumentation);

    // Shared data
    IL::ID packedToken;
    IL::ID compileType;
    IL::ID runtimeType;
    IL::ID runtimePUID;

    // Perform instrumentation check in PRE-block
    {
        IL::Emitter<> pre(program, *basicBlock);

        // Get global id of resource
        IL::ResourceTokenEmitter token(pre, resource);

        // Keep token
        packedToken = token.GetToken();

        // Get the ids
        compileType = pre.UInt32(static_cast<uint32_t>(compileTypeLiteral));
        runtimeType = token.GetType();
        runtimePUID = token.GetPUID();

        // Types must match, or out of bounds
        IL::ID cond = pre.Or(
            pre.NotEqual(compileType, runtimeType),
            pre.GreaterThanEqual(runtimePUID, pre.UInt32(IL::kResourceTokenPUIDInvalidStart))
        );

        // If so, branch to failure, otherwise resume
        pre.BranchConditional(
            cond,
            mismatchBlock, config.safeGuard ? matchBlock : resumeBlock,
            IL::ControlFlow::Selection(resumeBlock)
        );
    }

    // Export error in MISMATCH-block
    {
        IL::Emitter<> mismatch(program, *mismatchBlock);

        // Special PUIDs
        IL::ID isUndefined   = mismatch.Equal(runtimePUID, mismatch.UInt32(IL::kResourceTokenPUIDInvalidUndefined));
        IL::ID isOutOfBounds = mismatch.Equal(runtimePUID, mismatch.UInt32(IL::kResourceTokenPUIDInvalidOutOfBounds));

        // Setup message
        DescriptorMismatchMessage::ShaderExport msg;
        msg.sguid = mismatch.UInt32(sguid);
        msg.compileType = compileType;
        msg.runtimeType = runtimeType;
        msg.isUndefined = mismatch.Select(isUndefined, mismatch.UInt32(1), mismatch.UInt32(0));
        msg.isOutOfBounds = mismatch.Select(isOutOfBounds, mismatch.UInt32(1), mismatch.UInt32(0));
        
        // Detailed instrumentation?
        if (config.detail) {
            msg.chunks |= DescriptorMismatchMessage::Chunk::Detail;
            msg.detail.token = packedToken;
        }
        
        // Export the message
        mismatch.Export(exportID, msg);

        // If safe-guarded, allocate null fallback constant
        if (needsSafeGuardCFMerge) {
            safeGuardZero = program.GetConstants().FindConstantOrAdd(resultType, Backend::IL::NullConstant{})->id;
        }

        // Branch to resume
        mismatch.Branch(resumeBlock);
    }

    // If safe-guarded, phi the data back together
    if (needsSafeGuardCFMerge) {
        IL::Emitter<> resumeEmitter(program, *resumeBlock, instr);

        // Select the appropriate type with phi
        resumeEmitter.Phi(
            safeGuardValue,
            matchBlock, safeGuardRedirect,
            mismatchBlock, safeGuardZero
        );
    }

    // OK
    return config.safeGuard ? matchBlock->begin() : instr;
}

void DescriptorFeature::Inject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Options
    const SetInstrumentationConfigMessage config = FindOrDefault<SetInstrumentationConfigMessage>(specialization);

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext &context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        // Instruction of interest?
        switch (it->opCode) {
            default:
                return it;
            case IL::OpCode::LoadBuffer: {
                return InjectForResource(program, context.function, it, it->As<IL::LoadBufferInstruction>()->buffer, Backend::IL::ResourceTokenType::Buffer, config);
            }
            case IL::OpCode::StoreBuffer: {
                return InjectForResource(program, context.function, it, it->As<IL::StoreBufferInstruction>()->buffer, Backend::IL::ResourceTokenType::Buffer, config);
            }
            case IL::OpCode::StoreTexture: {
                return InjectForResource(program, context.function, it, it->As<IL::StoreTextureInstruction>()->texture, Backend::IL::ResourceTokenType::Texture, config);
            }
            case IL::OpCode::LoadTexture: {
                return InjectForResource(program, context.function, it, it->As<IL::LoadTextureInstruction>()->texture, Backend::IL::ResourceTokenType::Texture, config);
            }
            case IL::OpCode::SampleTexture: {
                auto* instr = it->As<IL::SampleTextureInstruction>();

                // Copy resources
                IL::ID texture = instr->texture;
                IL::ID sampler = instr->sampler;

                // Validate texture
                IL::BasicBlock::Iterator next = InjectForResource(program, context.function, it, texture, Backend::IL::ResourceTokenType::Texture, config);

                // Samplers are not guaranteed (can be combined)
                if (sampler == IL::InvalidID) {
                    return next;
                }

                // Validate sampler
                return InjectForResource(program, context.function, next, sampler, Backend::IL::ResourceTokenType::Sampler, config);
            }
        }
    });
}

FeatureInfo DescriptorFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Descriptor";
    info.description = "Instrumentation and validation of descriptor usage";
    return info;
}
