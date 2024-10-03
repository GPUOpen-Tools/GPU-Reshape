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
#include <Features/Waterfall/Feature.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/Analysis/SimulationAnalysis.h>
#include <Backend/IL/Analysis/DivergencePropagator.h>
#include <Backend/IL/Analysis/InterproceduralSimulationAnalysis.h>

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
    divergentResourceExportID = exportHost->Allocate<DivergentResourceIndexingMessage>();

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
    std::lock_guard guard(mutex);
    storage->AddStreamAndSwap(stream);
}

void WaterfallFeature::PreInject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Set up function simulators with divergence analysis
    for (IL::Function *function: program.GetFunctionList()) {
        if (auto&& analysis = function->GetAnalysisMap().FindPassOrAdd<IL::SimulationAnalysis>(program, *function)) {
            analysis->AddPropagator<IL::DivergencePropagator>(analysis->GetConstantPropagator(), program, *function);
        }
    }
    
    // Compute interprocedural analysis
    program.GetAnalysisMap().FindPassOrCompute<IL::InterproceduralSimulationAnalysis>(program);

    // Create data
    ComRef data = program.GetRegistry().AddNew<SharedData>();

    // Map all instructions of interest to their source blocks
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        switch (it->opCode) {
            default:
                return it;
            case IL::OpCode::AddressChain:
            case IL::OpCode::Extract: {
                data->instructionSourceBlocks[it->result] = context.basicBlock.GetID();
            }
        }
        return it;
    });
}

void WaterfallFeature::Inject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Options
    const auto config = CollapseOrDefault<SetInstrumentationConfigMessage>(specialization);

    // Get data
    ComRef data = program.GetRegistry().Get<SharedData>();

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        switch (it->opCode) {
            default:
                return it;
            case IL::OpCode::AddressChain:
                return InjectAddressChain(program, data, config, context, it);
            case IL::OpCode::Extract:
                return InjectExtract(program, data, context, it);
        }
    });
}

IL::BasicBlock::Iterator WaterfallFeature::InjectAddressChain(IL::Program& program, const ComRef<SharedData>& data, const SetInstrumentationConfigMessage& config, IL::VisitContext &context, IL::BasicBlock::Iterator it) {
    auto* instr = it->As<IL::AddressChainInstruction>();

    // Get composite type, must be pointer
    const auto* compositeType = program.GetTypeMap().GetType(instr->composite)->Cast<Backend::IL::PointerType>();
    if (!compositeType) {
        return it;
    }

    // Is this a function address-space indirection?
    const bool isFASIndirection = compositeType->addressSpace == Backend::IL::AddressSpace::Function;

    // Get the composite value type
    const Backend::IL::Type* compositeValueType = Backend::IL::GetTerminalValueType(program.GetTypeMap().GetType(instr->composite));

    // Addressing into a resource?
    const bool isResourceAddressing = Backend::IL::IsResourceType(compositeValueType);

    // Address chain indirection is primarily concerned with FAS indirections and resource addressing
    if (!isFASIndirection && !isResourceAddressing) {
        return it;
    }

    // Get the pre-injection analysis
    ComRef simulationAnalysis = context.function.GetAnalysisMap().FindPass<IL::SimulationAnalysis>();

    // Get the constant analysis
    const IL::ConstantPropagator& constantPropagator = simulationAnalysis->GetConstantPropagator();

    // Get the divergence propagator
    ComRef divergencePropagator = simulationAnalysis->FindPropagator<IL::DivergencePropagator>();

    // Outgoing message attributes
    uint32_t varyingOperandIndex = 0;

    // If this block is not executable, it's either unreachable or going to be DCE'd
    if (!simulationAnalysis->GetPropagationEngine().IsBlockExecutable(data->instructionSourceBlocks.at(program.GetIdentifierMap().GetSourceInstruction(it->result)))) {
        return it;
    }

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
        if (constantPropagator.IsConstant(instr->composite)) {
            return it;
        }

        // If the composite is not constant, we check each access chain
        bool anyChainVarying = false;
        bool anyChainDivergent = false;

        // Check if any part of the chain is varying or divergent
        for (uint32_t i = 0; i < instr->chains.count; i++) {
            const IL::AddressChain& chain = instr->chains[i];

            if (constantPropagator.IsVarying(chain.index)) {
                anyChainVarying     = true;
                varyingOperandIndex = i;
            }

            if (divergencePropagator->IsDivergent(chain.index)) {
                anyChainDivergent = true;
            }
        }

        // If none is varying, this can be collapsed
        // TODO: Check for partial constants!
        if (!anyChainVarying) {
            return it;
        }

        // If none is divergent, it can go through the M0 register with dynamic addressing
        if (!anyChainDivergent) {
            return it;
        }

        // Serial
        {
            std::lock_guard guard(mutex);

            // Export compile time message
            auto* message = MessageStreamView<WaterfallingConditionMessage>(stream).Add();
            message->sguid = sguidHost ? sguidHost->Bind(program, it) : InvalidShaderSGUID;
            message->varyingOperandIndex = varyingOperandIndex;
        }

        // No changes
        return it;
    } else {
        bool anyChainDivergent = false;

        // If this chain is annotated as divergent, do not check anything
        if (program.GetMetadataMap().HasMetadata(instr->result, IL::MetadataType::DivergentResourceIndex)) {
            return it;
        }

        // Resource addressing is concerned with divergence
        for (uint32_t i = 0; i < instr->chains.count; i++) {
            const IL::AddressChain& chain = instr->chains[i];
            anyChainDivergent |= divergencePropagator->GetDivergence(chain.index) == IL::WorkGroupDivergence::Divergent;
        }

        // If none are divergent, just fine
        if (!anyChainDivergent) {
            return it;
        }

        // Bind the SGUID
        ShaderSGUID sguid = sguidHost ? sguidHost->Bind(program, it) : InvalidShaderSGUID;

        // Allocate blocks
        IL::BasicBlock* resumeBlock = context.function.GetBasicBlocks().AllocBlock();
        IL::BasicBlock* divergentBlock = context.function.GetBasicBlocks().AllocBlock();

        // Split this basic block, move all instructions post and including the instrumented instruction to resume
        // ! iterator invalidated
        auto splitIt = context.basicBlock.Split(resumeBlock, it);

        // Get new chain instruction (moved after split)
        auto splitInstr = splitIt->As<IL::AddressChainInstruction>();

        // Perform instrumentation check
        IL::Emitter<> pre(program, context.basicBlock);
        {
            // Validate each chain index
            IL::ID anyRuntimeDivergent = InjectRuntimeDivergenceVisitor(program, pre, splitInstr);
        
            // If so, branch to failure, otherwise resume
            pre.BranchConditional(anyRuntimeDivergent, divergentBlock, resumeBlock, IL::ControlFlow::Selection(resumeBlock));
        }
        
        // Divergent indexing block
        IL::Emitter<> emitter(program, *divergentBlock);
        {
            emitter.AddBlockFlag(BasicBlockFlag::NoInstrumentation);

            // Setup message
            DivergentResourceIndexingMessage::ShaderExport msg;
            msg.sguid = emitter.UInt32(sguid);
            msg.pad = emitter.UInt32(0);
            emitter.Export(divergentResourceExportID, msg);

            // Branch back
            emitter.Branch(resumeBlock);
        }
        
        return splitIt;
    }
}

IL::ID WaterfallFeature::InjectRuntimeDivergenceVisitor(IL::Program& program, IL::Emitter<>& pre, const IL::AddressChainInstruction* splitInstr) {
    IL::ID anyRuntimeDivergent = IL::InvalidID;

    // Get the composite type
    const Backend::IL::Type* type = program.GetTypeMap().GetType(splitInstr->composite);

    // Collect divergence across the chains until we meet the resource
    for (uint32_t i = 0; i < splitInstr->chains.count; i++) {
        const IL::AddressChain& chain = splitInstr->chains[i];

        // Invalid if any lanes have a different values
        IL::ID anyRuntimeChainDivergent = pre.Not(pre.WaveAllEqual(chain.index));

        // Combine with last
        if (anyRuntimeDivergent != IL::InvalidID) {
            anyRuntimeDivergent = pre.Or(anyRuntimeDivergent, anyRuntimeChainDivergent);
        } else {
            anyRuntimeDivergent = anyRuntimeChainDivergent;
        }

        // If we've reached the resource itself, the next addressing is structural
        // or texel addressing, where variant indexing is completely fine
        if (IsResourceType(type)) {
            break;
        }

        // Get next type
        type = GetValueType(type);
    }

    // OK
    return anyRuntimeDivergent;
}

IL::BasicBlock::Iterator WaterfallFeature::InjectExtract(IL::Program &program, const ComRef<SharedData>& data, IL::VisitContext &context, IL::BasicBlock::Iterator it) {
    auto* instr = it->As<IL::ExtractInstruction>();

    // Get the pre-injection analysis
    ComRef simulationAnalysis = context.function.GetAnalysisMap().FindPass<IL::SimulationAnalysis>();

    // Get the constant analysis
    const IL::ConstantPropagator& constantPropagator = simulationAnalysis->GetConstantPropagator();

    // Get the divergence propagator
    ComRef divergencePropagator = simulationAnalysis->FindPropagator<IL::DivergencePropagator>();

    // If either the composite or index is constant, no conditional masking will take place
    if (constantPropagator.IsConstant(instr->composite)) {
        return it;
    }

    // Outgoing message attributes
    uint32_t varyingOperandIndex = 0;
    
    // If the composite is not constant, we check each access chain
    bool anyChainVarying = false;
    bool anyChainDivergent = false;

    // Check if any part of the chain is varying or divergent
    for (uint32_t i = 0; i < instr->chains.count; i++) {
        const IL::ExtractChain& chain = instr->chains[i];

        if (constantPropagator.IsVarying(chain.index)) {
            anyChainVarying     = true;
            varyingOperandIndex = i;
        }

        if (divergencePropagator->IsDivergent(chain.index)) {
            anyChainDivergent = true;
        }
    }
    
    // If none is varying, this can be collapsed
    // TODO: Check for partial constants!
    if (!anyChainVarying) {
        return it;
    }

    // If none is divergent, it can go through the M0 register with dynamic addressing
    if (!anyChainDivergent) {
        return it;
    }
    
    // TODO: Configurable masking limits
    
    // Bind the current sguid
    ShaderSGUID sguid = sguidHost ? sguidHost->Bind(program, it) : InvalidShaderSGUID;

    // Serial
    {
        std::lock_guard guard(mutex);

        // Export compile time message
        auto* message = MessageStreamView<WaterfallingConditionMessage>(stream).Add();
        message->sguid = sguid;
        message->varyingOperandIndex = varyingOperandIndex;
    }

    return it;
}

FeatureInfo WaterfallFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Waterfall";
    info.description = "Instrumentation and validation of address scalarization / waterfalling";
    return info;
}


