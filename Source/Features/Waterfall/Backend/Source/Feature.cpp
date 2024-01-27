// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
#include <Features/Waterfall//Feature.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/Analysis/ConstantAnalysis.h>

// Generated schema
#include <Schemas/Features/Waterfall.h>
#include <Schemas/Instrumentation.h>
#include <Schemas/InstrumentationCommon.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStreamCommon.h>

// Common
#include <Common/Registry.h>
#include <Common/Sink.h>

bool WaterfallFeature::Install() {
    // Must have the export host
    auto exportHost = registry->Get<IShaderExportHost>();
    if (!exportHost) {
        return false;
    }

    // Allocate the shared export
    exportID = exportHost->Allocate<WaterfallingConditionMessage>();

    // Optional sguid host
    sguidHost = registry->Get<IShaderSGUIDHost>();

    // OK
    return true;
}

FeatureHookTable WaterfallFeature::GetHookTable() {
    return FeatureHookTable{};
}

void WaterfallFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void WaterfallFeature::CollectMessages(IMessageStorage *storage) {
    storage->AddStreamAndSwap(stream);
}

void WaterfallFeature::Inject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Options
    [[maybe_unused]]
    const auto config = CollapseOrDefault<SetInstrumentationConfigMessage>(specialization);

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        switch (it->opCode) {
            default:
                return it;
            case IL::OpCode::AddressChain:
                return InjectAddressChain(program, context, it);
            case IL::OpCode::Extract:
                return InjectExtract(program, context, it);
        }
    });
}

IL::BasicBlock::Iterator WaterfallFeature::InjectAddressChain(IL::Program& program, IL::VisitContext &context, IL::BasicBlock::Iterator it) {
    auto* instr = it->As<IL::AddressChainInstruction>();

    // Get composite type, must be pointer
    const auto* compositeType = program.GetTypeMap().GetType(instr->composite)->Cast<Backend::IL::PointerType>();
    if (!compositeType) {
        return it;
    }

    // Is this a function address-space indirection?
    const bool isFASIndirection = compositeType->addressSpace == Backend::IL::AddressSpace::Function;

    // Get the composite value type
    const Backend::IL::Type* compositeValueType = Backend::IL::GetValueType(program.GetTypeMap().GetType(instr->composite));

    // Addressing into a resource?
    const bool isResourceAddressing = Backend::IL::IsResourceType(compositeValueType);

    // Address chain indirection is primarily concerned with FAS indirections and resource addressing
    if (!isFASIndirection && !isResourceAddressing) {
        return it;
    }

    // TODO: Move analysis outside!

    // Computer all dominators
    IL::DominatorAnalysis dominatorAnalysis(context.function.GetBasicBlocks());
    dominatorAnalysis.Compute();

    // Compute all loops
    IL::LoopAnalysis loopAnalysis(dominatorAnalysis);
    loopAnalysis.Compute();

    // Compute all constants
    IL::ConstantAnalysis constantAnalysis(program, dominatorAnalysis, loopAnalysis);
    constantAnalysis.Compute(context.function.GetBasicBlocks());

    // FAS based checks?
    if (isFASIndirection) {
        // Addressing vector values can always be compiled to a set of conditional masks
        if (compositeValueType->Is<Backend::IL::VectorType>()) {
            // TODO: Configurable masking limits
        }

        // Addressing array values can be compiled to a set of conditional masks for
        // suitably small arrays. Really, it's the same as vectors, it's all scalar
        // execution engines anyway.
        if (compositeValueType->Is<Backend::IL::ArrayType>()) {
            // TODO: Configurable masking limits
        }

        // Addressing matrix types is typically only compiled to a set conditional masks
        // if only a single dimension is varying.
        if (compositeValueType->Is<Backend::IL::MatrixType>()) {
            // TODO: Configurable masking limits and dimensionality checks
        }

        // If the base composite is constant, this will never waterfall
        // The resulting data is either inlined or moved to memory
        if (constantAnalysis.IsConstant(instr->composite)) {
            return it;
        }

        // If the composite is not constant, we check each access chain
        bool anyChainVarying = false;

        // Check if any part of the chain is varying
        for (uint32_t i = 0; i < instr->chains.count; i++) {
            const IL::AddressChain& chain = instr->chains[i];
            anyChainVarying |= constantAnalysis.IsVarying(chain.index);
        }

        // If none is varying, this can be collapsed
        // TODO: Check for partial constants!
        if (!anyChainVarying) {
            return it;
        }
    } else {
        // Resource addressing is concerned with divergence
        // This is incomplete
#if 0
        bool anyChainNonUniform = false;

        for (uint32_t i = 0; i < instr->chains.count; i++) {
            const IL::AddressChain& chain = instr->chains[i];
            anyChainNonUniform |= divergenceAnalysis.GetDivergence(chain.index) == IL::WorkGroupDivergence::Divergent;
        }

        if (!anyChainNonUniform) {
            return it;
        }
#endif // 0
        
        return it;
    }

    // Bind the current sguid
    ShaderSGUID sguid = sguidHost ? sguidHost->Bind(program, it) : InvalidShaderSGUID;

    // Insert just prior to the address chain
    IL::Emitter<> emitter(context.program, context.basicBlock, it);

    // Export waterfalling condition
    WaterfallingConditionMessage::ShaderExport msg;
    msg.sguid = emitter.UInt32(sguid);
    msg.pad = emitter.UInt32(0);
    emitter.Export(exportID, msg);

    return emitter.GetIterator();
}

IL::BasicBlock::Iterator WaterfallFeature::InjectExtract(IL::Program &program, IL::VisitContext &context, IL::BasicBlock::Iterator it) {
    auto* instr = it->As<IL::ExtractInstruction>();

    // TODO: Move analysis outside!
    
    // Computer all dominators
    IL::DominatorAnalysis dominatorAnalysis(context.function.GetBasicBlocks());
    dominatorAnalysis.Compute();

    // Compute all loops
    IL::LoopAnalysis loopAnalysis(dominatorAnalysis);
    loopAnalysis.Compute();

    // Compute all constants
    IL::ConstantAnalysis constantAnalysis(program, dominatorAnalysis, loopAnalysis);
    constantAnalysis.Compute(context.function.GetBasicBlocks());

    // If either the composite or index is constant, no conditional masking will take place
    if (constantAnalysis.IsConstant(instr->composite) || constantAnalysis.IsConstant(instr->index)) {
        return it;
    }

    // TODO: Configurable masking limits
    
    // Bind the current sguid
    ShaderSGUID sguid = sguidHost ? sguidHost->Bind(program, it) : InvalidShaderSGUID;

    // Insert just prior to the extract
    IL::Emitter<> emitter(context.program, context.basicBlock, it);

    // Export waterfalling condition
    WaterfallingConditionMessage::ShaderExport msg;
    msg.sguid = emitter.UInt32(sguid);
    msg.pad = emitter.UInt32(0);
    emitter.Export(exportID, msg);

    return emitter.GetIterator();
}

FeatureInfo WaterfallFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Waterfall";
    info.description = "";
    return info;
}


