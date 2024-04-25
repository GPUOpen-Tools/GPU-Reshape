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

// Feature
#include <Features/ExportIndices//Feature.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/ResourceTokenEmitter.h>

// Generated schema
#include <Schemas/Features/ExportIndices.h>
#include <Schemas/Instrumentation.h>
#include <Schemas/InstrumentationCommon.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStreamCommon.h>

// Common
#include <Common/Registry.h>

bool ExportIndicesFeature::Install() {
    // Must have the export host
    auto exportHost = registry->Get<IShaderExportHost>();
    if (!exportHost) {
        return false;
    }

    // Allocate the shared export
    exportID = exportHost->Allocate<InefficientExportMessage>();

    // Optional sguid host
    sguidHost = registry->Get<IShaderSGUIDHost>();

    // OK
    return true;
}

FeatureHookTable ExportIndicesFeature::GetHookTable() {
    return FeatureHookTable{};
}

void ExportIndicesFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void ExportIndicesFeature::CollectMessages(IMessageStorage *storage) {
    storage->AddStreamAndSwap(stream);
}

void ExportIndicesFeature::Inject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Options
    const SetInstrumentationConfigMessage config = CollapseOrDefault<SetInstrumentationConfigMessage>(specialization);

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        // Instruction of interest?
        IL::ID outputIndex;
        IL::ID resource = IL::InvalidID;
        switch (it->opCode) {
            default:
                return it;
            case IL::OpCode::StoreVertexOutput:
                outputIndex = it->As<IL::StoreVertexOutputInstruction>()->vertexIndex;
                break;
            case IL::OpCode::StorePrimitiveOutput:
                outputIndex = it->As<IL::StorePrimitiveOutputInstruction>()->primitiveIndex;
                break;
        }

        // Get type
        //const Backend::IL::Type *valueType = program.GetTypeMap().GetType(outputIndex);

        // Get index variable
        const Backend::IL::Variable *outputIndexVariable = program.GetVariableList().GetVariable(outputIndex);

        // Instrumentation Segmentation
        //
        //             BEFORE                                 AFTER
        //
        //   ┌─────┬─────────────┬───────┐      ┌─────┐                   ┌─────────────┬──────┐
        //   │     │             │       │      │     │        OK         │             │      │
        //   │ Pre │ Instruction │ Post  │      │ Pre ├───────────────────┤ Instruction │ Post │
        //   │     │             │       │      │     │                   │   [RESUME]  │      │
        //   └─────┴─────────────┴───────┘      └──┬──┘                   └──────┬──────┴──────┘
        //                                         │    ┌────────────┐           │
        //                                     INV │    │            │           │
        //                                         └────┤ noThreadID ├───────────┘
        //                                              │            │
        //                                              └────────────┘

        // Bind the SGUID
        ShaderSGUID sguid = sguidHost ? sguidHost->Bind(program, it) : InvalidShaderSGUID;

        // Allocate resume
        IL::BasicBlock* resumeBlock = context.function.GetBasicBlocks().AllocBlock();

        // Split this basic block, move all instructions post and including the instrumented instruction to resume
        // ! iterator invalidated
        auto instr = context.basicBlock.Split(resumeBlock, it);

        // Perform instrumentation check
        IL::Emitter<> pre(program, context.basicBlock);

        // Failure conditions
        // IL::ID isInf = pre.Any(pre.IsInf(value));
        // IL::ID isNaN = pre.Any(pre.IsNaN(value));

        // Failure conditions: output index is not thread ID
        IL::ID isNotThreadIndex = pre.Any(pre.Equal(outputIndex, pre.FlattenedLocalThreadIndex()));
        //IL::ID isDivergent = pre.Any(pre.WaveAllEqual(pre.Sub(outputIndex, pre.FlattenedLocalThreadIndex())));

        // isInf/isNaN block
        IL::Emitter<> oob(program, *context.function.GetBasicBlocks().AllocBlock());
        oob.AddBlockFlag(BasicBlockFlag::NoInstrumentation);

        // Setup message
        InefficientExportMessage::ShaderExport msg;
        msg.sguid = oob.UInt32(sguid);
        // TODO
        msg.isNoThreadID = oob.Select(false, oob.UInt32(1), oob.UInt32(0));

        // Detailed instrumentation?
        if (config.detail && resource != IL::InvalidID) {
            msg.chunks |= InefficientExportMessage::Chunk::Detail;
            msg.detail.token = IL::ResourceTokenEmitter(oob, resource).GetToken();
        }

        // Export the message
        oob.Export(exportID, msg);

        // Branch back
        oob.Branch(resumeBlock);

        // If so, branch to failure, otherwise resume
        pre.BranchConditional(isNotThreadIndex, oob.GetBasicBlock(), resumeBlock, IL::ControlFlow::Selection(resumeBlock));
        return instr;
    });
}

FeatureInfo ExportIndicesFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Export Indices";
    info.description = "Instrumentation and validation of export indices for vertices and primitives in mesh shader";
    return info;
}


