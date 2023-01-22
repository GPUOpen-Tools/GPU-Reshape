#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockFunction.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Compiler/SpvParseContext.h>
#include <Backends/Vulkan/Compiler/SpvJob.h>

// Backend
#include <Backend/IL/Emitter.h>

// Common
#include <Common/Alloca.h>
#include <Common/Containers/TrivialStackVector.h>

void SpvPhysicalBlockFunction::Parse() {
    block = table.scan.GetPhysicalBlock(SpvPhysicalBlockType::Function);

    // Parse instructions
    SpvParseContext ctx(block->source);
    while (ctx) {
        // Line is allowed before definition
        if (ctx->GetOp() == SpvOpLine) {
            // Skip for now
            ctx.Next();
        }

        // Must be opening
        ASSERT(ctx->GetOp() == SpvOpFunction, "Unexpected instruction");

        // Attempt to get existing function in case it's prototyped
        IL::Function *function = program.GetFunctionList().GetFunction(ctx.GetResult());

        // Allocate a new one if need be
        if (!function) {
            function = program.GetFunctionList().AllocFunction(ctx.GetResult());

            // Ignore control (this will bite you later)
            ctx++;

            // Set the function type
            function->SetFunctionType(table.typeConstantVariable.typeMap.GetTypeFromId(ctx++)->As<Backend::IL::FunctionType>());
        }

        // Next instruction
        ctx.Next();

        // Parse header
        ParseFunctionHeader(function, ctx);

        // Any body?
        if (ctx->GetOp() != SpvOpFunctionEnd) {
            // Parse the body
            ParseFunctionBody(function, ctx);

            // Perform post patching
            PostPatchLoopContinue(function);
        }

        // Must be body
        ASSERT(ctx->GetOp() == SpvOpFunctionEnd, "Expected function end");
        ctx.Next();
    }
}

void SpvPhysicalBlockFunction::ParseFunctionHeader(IL::Function *function, SpvParseContext &ctx) {
    while (ctx) {
        // Create type association
        table.typeConstantVariable.AssignTypeAssociation(ctx);

        // Handle instruction
        switch (ctx->GetOp()) {
            default: {
                ASSERT(false, "Unexpected instruction in function header");
                return;
            }
            case SpvOpLine:
            case SpvOpNoLine: {
                break;
            }
            case SpvOpLabel: {
                // Not interested
                return;
            }
            case SpvOpFunctionParameter: {
                Backend::IL::Variable variable;
                variable.addressSpace = Backend::IL::AddressSpace::Function;
                variable.type = table.typeConstantVariable.typeMap.GetTypeFromId(ctx.GetResultType());
                variable.id = ctx.GetResult();
                function->GetParameters().Add(variable);
                break;
            }
        }

        // Next instruction
        ctx.Next();
    }
}

void SpvPhysicalBlockFunction::ParseFunctionBody(IL::Function *function, SpvParseContext &ctx) {
    // Current basic block
    IL::BasicBlock *basicBlock{nullptr};

    // Current control flow
    IL::BranchControlFlow controlFlow{};

    // Current source association
    SpvSourceAssociation sourceAssociation{};

    // Parse all instructions
    while (ctx && ctx->GetOp() != SpvOpFunctionEnd) {
        IL::Source source = ctx.Source();

        // Create type association
        table.typeConstantVariable.AssignTypeAssociation(ctx);

        // Create source association
        if (sourceAssociation) {
            table.debugStringSource.sourceMap.AddSourceAssociation(source.codeOffset, sourceAssociation);
        }

        // Handle instruction
        switch (ctx->GetOp()) {
            case SpvOpLabel: {
                // Terminate current basic block
                if (basicBlock) {
                    /* */
                }

                // Allocate a new basic block
                basicBlock = function->GetBasicBlocks().AllocBlock(ctx.GetResult());
                break;
            }

            case SpvOpLine: {
                sourceAssociation.fileUID = table.debugStringSource.sourceMap.GetFileIndex(ctx++);
                sourceAssociation.line = (ctx++) - 1;
                sourceAssociation.column = (ctx++) - 1;

                if (sourceAssociation.column == UINT16_MAX) {
                    sourceAssociation.column = 0;
                }
                break;
            }

            case SpvOpNoLine: {
                sourceAssociation = {};
                break;
            }

            case SpvOpLoad: {
                // Append
                IL::LoadInstruction instr{};
                instr.opCode = IL::OpCode::Load;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.address = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpStore: {
                // Append
                IL::StoreInstruction instr{};
                instr.opCode = IL::OpCode::Store;
                instr.result = IL::InvalidID;
                instr.source = source;
                instr.address = ctx++;
                instr.value = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpFAdd:
            case SpvOpIAdd: {
                // Append
                IL::AddInstruction instr{};
                instr.opCode = IL::OpCode::Add;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.lhs = ctx++;
                instr.rhs = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpFSub:
            case SpvOpISub: {
                // Append
                IL::SubInstruction instr{};
                instr.opCode = IL::OpCode::Sub;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.lhs = ctx++;
                instr.rhs = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpFDiv:
            case SpvOpSDiv:
            case SpvOpUDiv: {
                // Append
                IL::DivInstruction instr{};
                instr.opCode = IL::OpCode::Div;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.lhs = ctx++;
                instr.rhs = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpFMul:
            case SpvOpIMul: {
                // Append
                IL::MulInstruction instr{};
                instr.opCode = IL::OpCode::Mul;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.lhs = ctx++;
                instr.rhs = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpLogicalAnd: {
                // Append
                IL::AndInstruction instr{};
                instr.opCode = IL::OpCode::And;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.lhs = ctx++;
                instr.rhs = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpLogicalOr: {
                // Append
                IL::OrInstruction instr{};
                instr.opCode = IL::OpCode::Or;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.lhs = ctx++;
                instr.rhs = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpLogicalEqual:
            case SpvOpIEqual:
            case SpvOpFOrdEqual:
            case SpvOpFUnordEqual: {
                // Append
                IL::EqualInstruction instr{};
                instr.opCode = IL::OpCode::Equal;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.lhs = ctx++;
                instr.rhs = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpLogicalNotEqual:
            case SpvOpINotEqual:
            case SpvOpFOrdNotEqual:
            case SpvOpFUnordNotEqual: {
                // Append
                IL::NotEqualInstruction instr{};
                instr.opCode = IL::OpCode::NotEqual;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.lhs = ctx++;
                instr.rhs = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpSLessThan:
            case SpvOpULessThan:
            case SpvOpFOrdLessThan:
            case SpvOpFUnordLessThan: {
                // Append
                IL::LessThanInstruction instr{};
                instr.opCode = IL::OpCode::LessThan;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.lhs = ctx++;
                instr.rhs = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpSLessThanEqual:
            case SpvOpULessThanEqual:
            case SpvOpFOrdLessThanEqual:
            case SpvOpFUnordLessThanEqual: {
                // Append
                IL::LessThanEqualInstruction instr{};
                instr.opCode = IL::OpCode::LessThanEqual;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.lhs = ctx++;
                instr.rhs = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpSGreaterThan:
            case SpvOpUGreaterThan:
            case SpvOpFOrdGreaterThan:
            case SpvOpFUnordGreaterThan: {
                // Append
                IL::GreaterThanInstruction instr{};
                instr.opCode = IL::OpCode::GreaterThan;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.lhs = ctx++;
                instr.rhs = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpSGreaterThanEqual:
            case SpvOpUGreaterThanEqual:
            case SpvOpFOrdGreaterThanEqual:
            case SpvOpFUnordGreaterThanEqual: {
                // Append
                IL::GreaterThanEqualInstruction instr{};
                instr.opCode = IL::OpCode::GreaterThanEqual;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.lhs = ctx++;
                instr.rhs = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpIsInf: {
                // Append
                IL::IsInfInstruction instr{};
                instr.opCode = IL::OpCode::IsInf;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.value = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpIsNan: {
                // Append
                IL::IsNaNInstruction instr{};
                instr.opCode = IL::OpCode::IsNaN;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.value = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpSelect: {
                // Append
                IL::SelectInstruction instr{};
                instr.opCode = IL::OpCode::Select;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.condition = ctx++;
                instr.pass = ctx++;
                instr.fail = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpBitwiseOr: {
                // Append
                IL::BitOrInstruction instr{};
                instr.opCode = IL::OpCode::BitOr;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.lhs = ctx++;
                instr.rhs = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpBitwiseAnd: {
                // Append
                IL::BitAndInstruction instr{};
                instr.opCode = IL::OpCode::BitAnd;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.lhs = ctx++;
                instr.rhs = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpShiftLeftLogical: {
                // Append
                IL::BitShiftLeftInstruction instr{};
                instr.opCode = IL::OpCode::BitShiftLeft;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.value = ctx++;
                instr.shift = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpShiftRightLogical:
            case SpvOpShiftRightArithmetic: {
                // Append
                IL::BitShiftRightInstruction instr{};
                instr.opCode = IL::OpCode::BitShiftRight;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.value = ctx++;
                instr.shift = ctx++;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpBranch: {
                // Append
                // NOTE: Always marked as modified, to re-emit the CFG
                IL::BranchInstruction instr{};
                instr.opCode = IL::OpCode::Branch;
                instr.result = IL::InvalidID;
                instr.source = source.Modify();
                instr.branch = ctx++;

                // Consume control flow
                instr.controlFlow = controlFlow;
                controlFlow = {};

                // Append
                IL::InstructionRef<IL::BranchInstruction> ref = basicBlock->Append(instr);

                // Loop?
                if (instr.controlFlow._continue != IL::InvalidID) {
                    LoopContinueBlock loopContinueBlock;
                    loopContinueBlock.instruction = ref;
                    loopContinueBlock.block = instr.controlFlow._continue;
                    loopContinueBlocks.push_back(loopContinueBlock);
                }
                break;
            }

            case SpvOpSelectionMerge: {
                controlFlow.merge = ctx++;
                controlFlow._continue = IL::InvalidID;
                break;
            }

            case SpvOpLoopMerge: {
                controlFlow.merge = ctx++;
                controlFlow._continue = ctx++;
                break;
            }

            case SpvOpBranchConditional: {
                // Append
                // NOTE: Always marked as modified, to re-emit the CFG
                IL::BranchConditionalInstruction instr{};
                instr.opCode = IL::OpCode::BranchConditional;
                instr.result = IL::InvalidID;
                instr.source = source.Modify();
                instr.cond = ctx++;
                instr.pass = ctx++;
                instr.fail = ctx++;

                // Consume control flow
                instr.controlFlow = controlFlow;
                controlFlow = {};

                // Append
                IL::InstructionRef<IL::BranchConditionalInstruction> ref = basicBlock->Append(instr);

                // Loop?
                if (instr.controlFlow._continue != IL::InvalidID) {
                    LoopContinueBlock loopContinueBlock;
                    loopContinueBlock.instruction = ref;
                    loopContinueBlock.block = instr.controlFlow._continue;
                    loopContinueBlocks.push_back(loopContinueBlock);
                }
                break;
            }

            case SpvOpSwitch: {
                // Determine number of cases
                const uint32_t caseCount = (ctx->GetWordCount() - 3) / 2;
                ASSERT((ctx->GetWordCount() - 3) % 2 == 0, "Unexpected case word count");

                // Create instruction
                auto* instr = ALLOCA_SIZE(IL::SwitchInstruction, IL::SwitchInstruction::GetSize(caseCount));
                instr->opCode = IL::OpCode::Switch;
                instr->result = IL::InvalidID;
                instr->source = source.Modify();
                instr->value = ctx++;
                instr->_default = ctx++;
                instr->cases.count = caseCount;

                // Consume control flow
                instr->controlFlow = controlFlow;
                controlFlow = {};

                // Fill cases
                for (uint32_t i = 0; i < caseCount; i++) {
                    IL::SwitchCase _case;
                    _case.literal = ctx++;
                    _case.branch = ctx++;
                    instr->cases[i] = _case;
                }

                basicBlock->Append(instr);
                break;
            }

            case SpvOpPhi: {
                // Determine number of cases
                const uint32_t valueCount = (ctx->GetWordCount() - 3) / 2;
                ASSERT((ctx->GetWordCount() - 3) % 2 == 0, "Unexpected value word count");

                // Create instruction
                auto* instr = ALLOCA_SIZE(IL::PhiInstruction, IL::PhiInstruction::GetSize(valueCount));
                instr->opCode = IL::OpCode::Phi;
                instr->result = ctx.GetResult();
                instr->source = source;
                instr->values.count = valueCount;

                // Fill cases
                for (uint32_t i = 0; i < valueCount; i++) {
                    IL::PhiValue value;
                    value.value = ctx++;
                    value.branch = ctx++;
                    instr->values[i] = value;
                }

                // Append dynamic
                basicBlock->Append(instr);
                break;
            }

            case SpvOpVariable: {
                const Backend::IL::Type *type = table.typeConstantVariable.typeMap.GetTypeFromId(ctx.GetResultType());

                // Variables may only appear in the first block of a function, there's no co-dependence so just insert them
                function->GetVariables().Add(Backend::IL::Variable {
                    .id = ctx.GetResult(),
                    .addressSpace = Backend::IL::AddressSpace::Function,
                    .type = type
                });

                program.GetTypeMap().SetType(ctx.GetResult(), type);
                break;
            }

                // Integral literal
            case SpvOpConstant: {
                uint32_t value = ctx++;

                // Append
                IL::LiteralInstruction instr{};
                instr.opCode = IL::OpCode::Literal;
                instr.result = ctx.GetResult();
                instr.source = source;
                instr.type = IL::LiteralType::Int;
                instr.signedness = true;
                instr.bitWidth = 32;
                instr.value.integral = value;
                basicBlock->Append(instr);
                break;
            }

                // Image store operation, fx. texture & buffer writes
            case SpvOpImageRead:
            case SpvOpImageFetch: {
                uint32_t image = ctx++;
                uint32_t coordinate = ctx++;

                const Backend::IL::Type *type = program.GetTypeMap().GetType(image);

                if (type->kind == Backend::IL::TypeKind::Buffer) {
                    // Append
                    IL::LoadBufferInstruction instr{};
                    instr.opCode = IL::OpCode::LoadBuffer;
                    instr.result = ctx.GetResult();
                    instr.source = source;
                    instr.buffer = image;
                    instr.index = coordinate;
                    basicBlock->Append(instr);
                } else {
                    // Append
                    IL::LoadTextureInstruction instr{};
                    instr.opCode = IL::OpCode::LoadTexture;
                    instr.result = ctx.GetResult();
                    instr.source = source;
                    instr.texture = image;
                    instr.index = coordinate;
                    basicBlock->Append(instr);
                }
                break;
            }

                // Image store operation, fx. texture & buffer writes
            case SpvOpImageWrite: {
                uint32_t image = ctx++;
                uint32_t coordinate = ctx++;
                uint32_t texel = ctx++;

                const Backend::IL::Type *type = program.GetTypeMap().GetType(image);

                if (type->kind == Backend::IL::TypeKind::Buffer) {
                    // Append
                    IL::StoreBufferInstruction instr{};
                    instr.opCode = IL::OpCode::StoreBuffer;
                    instr.result = IL::InvalidID;
                    instr.source = source;
                    instr.buffer = image;
                    instr.index = coordinate;
                    instr.value = texel;
                    instr.mask = IL::ComponentMask::All;
                    basicBlock->Append(instr);
                } else {
                    // Append
                    IL::StoreTextureInstruction instr{};
                    instr.opCode = IL::OpCode::StoreTexture;
                    instr.result = IL::InvalidID;
                    instr.source = source;
                    instr.texture = image;
                    instr.index = coordinate;
                    instr.texel = texel;
                    instr.mask = IL::ComponentMask::All;
                    basicBlock->Append(instr);
                }
                break;
            }

            case SpvOpReturn: {
                IL::ReturnInstruction instr{};
                instr.opCode = IL::OpCode::Return;
                instr.result = IL::InvalidID;
                instr.source = source;
                instr.value = IL::InvalidID;
                basicBlock->Append(instr);
                break;
            }

            case SpvOpReturnValue: {
                IL::ReturnInstruction instr{};
                instr.opCode = IL::OpCode::Return;
                instr.result = IL::InvalidID;
                instr.source = source;
                instr.value = ctx++;
                basicBlock->Append(instr);
                break;
            }

            default: {
                if (basicBlock) {
                    // Emit as unexposed
                    IL::UnexposedInstruction instr{};
                    instr.opCode = IL::OpCode::Unexposed;
                    instr.result = ctx.HasResult() ? ctx.GetResult() : IL::InvalidID;
                    instr.source = source;
                    instr.backendOpCode = ctx->GetOp();
                    basicBlock->Append(instr);
                }
                break;
            }
        }

        // Next instruction
        ctx.Next();
    }
}

bool SpvPhysicalBlockFunction::Compile(const SpvJob& job, SpvIdMap &idMap) {
    // Create data associations
    CreateDataResourceMap(job);
    CreateDataPCMap(job);

    // Compile all function declarations
    for (IL::Function* fn : program.GetFunctionList()) {
        if (!CompileFunction(idMap, *fn, true)) {
            return false;
        }
    }

    // Compile all function definitions
#if 0
    for (IL::Function& fn : program.GetFunctionList()) {
            if (!CompileFunction(idMap, fn, true)) {
                return false;
            }
        }
#endif

    // OK
    return true;
}

bool SpvPhysicalBlockFunction::CompileFunction(SpvIdMap &idMap, IL::Function &fn, bool emitDefinition) {
    const Backend::IL::FunctionType* type = fn.GetFunctionType();
    ASSERT(type, "Function without a given type");

    // Emit function open
    SpvInstruction& spvFn = block->stream.Allocate(SpvOpFunction, 5);
    spvFn[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(type->returnType);
    spvFn[2] = fn.GetID();
    spvFn[3] = SpvFunctionControlMaskNone;
    spvFn[4] = table.typeConstantVariable.typeMap.GetSpvTypeId(type);

    // Generate parameters
    for (const Backend::IL::Variable& parameter : fn.GetParameters()) {
        SpvInstruction& spvParam = block->stream.Allocate(SpvOpFunctionParameter, 3);
        spvParam[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(parameter.type);
        spvParam[2] = parameter.id;
    }

    // Compile all basic blocks if the definition is being emitted
    if (emitDefinition) {
        bool isModifiedScope = false;

        // Check if any child block is modified
        for (IL::BasicBlock* basicBlock : fn.GetBasicBlocks()) {
            isModifiedScope |= basicBlock->IsModified();
        }

        for (IL::BasicBlock* basicBlock : fn.GetBasicBlocks()) {
            if (!CompileBasicBlock(idMap, fn, basicBlock, isModifiedScope)) {
                return false;
            }
        }
    }

    // Emit function close
    block->stream.Allocate(SpvOpFunctionEnd, 1);

    // OK
    return true;
}

bool SpvPhysicalBlockFunction::CompileBasicBlock(SpvIdMap &idMap, IL::Function& fn, IL::BasicBlock *bb, bool isModifiedScope) {
    SpvStream& stream = block->stream;

    Backend::IL::TypeMap& ilTypeMap = program.GetTypeMap();

    // Emit label
    SpvInstruction& label = stream.Allocate(SpvOpLabel, 2);
    label[1] = bb->GetID();

    // First block?
    if (bb == *fn.GetBasicBlocks().begin()) {
        // Emit all variables, order doesn't matter
        for (const Backend::IL::Variable& variable : fn.GetVariables()) {
            SpvInstruction& spv = stream.Allocate(SpvOpVariable, 4);
            spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(variable.type);
            spv[2] = variable.id;
            spv[3] = SpvStorageClassFunction;
        }

        // Has the function been modified?
        if (isModifiedScope) {
            // Create user data ids
            CreateDataLookups(block->stream, idMap);
        }
    }

    // Emit all backend instructions
    for (auto instr = bb->begin(); instr != bb->end(); instr++) {
        // If trivial, just copy it directly
        if (instr->source.TriviallyCopyable()) {
            stream.Template(instr->source);
            continue;
        }

        // Result type of the instruction
        const Backend::IL::Type* resultType = nullptr;
        if (instr->result != IL::InvalidID) {
            resultType = ilTypeMap.GetType(instr->result);
        }

        switch (instr->opCode) {
            default: {
                ASSERT(false, "Invalid instruction in basic block");
                return false;
            }
            case IL::OpCode::Unexposed: {
                ASSERT(false, "Non trivially copyable unexposed instruction");
                break;
            }
            case IL::OpCode::Literal: {
                auto *literal = instr.As<IL::LiteralInstruction>();

                SpvInstruction& spv = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = literal->result;
                spv[3] = static_cast<uint32_t>(literal->value.integral);
                break;
            }
            case IL::OpCode::LoadTexture: {
                auto *loadTexture = instr.As<IL::LoadTextureInstruction>();

                // Get the texture type
                const auto* textureType = ilTypeMap.GetType(loadTexture->texture)->As<Backend::IL::TextureType>();

                // Load image
                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpImageRead, 5, instr->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(textureType->sampledType);
                spv[2] = loadTexture->result;
                spv[3] = idMap.Get(loadTexture->texture);
                spv[4] = idMap.Get(loadTexture->index);
                break;
            }
            case IL::OpCode::StoreTexture: {
                auto *storeTexture = instr.As<IL::StoreTextureInstruction>();

                // Write image
                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpImageWrite, 4, instr->source);
                spv[1] = storeTexture->texture;
                spv[2] = idMap.Get(storeTexture->index);
                spv[3] = idMap.Get(storeTexture->texel);
                break;
            }
            case IL::OpCode::Add: {
                auto *add = instr.As<IL::AddInstruction>();

                SpvOp op = resultType->kind == Backend::IL::TypeKind::FP ? SpvOpFAdd : SpvOpIAdd;

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, add->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = add->result;
                spv[3] = idMap.Get(add->lhs);
                spv[4] = idMap.Get(add->rhs);
                break;
            }
            case IL::OpCode::Sub: {
                auto *add = instr.As<IL::SubInstruction>();

                SpvOp op = resultType->kind == Backend::IL::TypeKind::FP ? SpvOpFSub : SpvOpISub;

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, add->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = add->result;
                spv[3] = idMap.Get(add->lhs);
                spv[4] = idMap.Get(add->rhs);
                break;
            }
            case IL::OpCode::Div: {
                auto *add = instr.As<IL::DivInstruction>();

                SpvOp op;
                if (resultType->kind == Backend::IL::TypeKind::Int) {
                    op = resultType->As<Backend::IL::IntType>()->signedness ? SpvOpSDiv : SpvOpUDiv;
                } else {
                    op = SpvOpFDiv;
                }

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, add->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = add->result;
                spv[3] = idMap.Get(add->lhs);
                spv[4] = idMap.Get(add->rhs);
                break;
            }
            case IL::OpCode::Mul: {
                auto *add = instr.As<IL::MulInstruction>();

                SpvOp op = resultType->kind == Backend::IL::TypeKind::FP ? SpvOpFMul : SpvOpIMul;

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, add->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = add->result;
                spv[3] = idMap.Get(add->lhs);
                spv[4] = idMap.Get(add->rhs);
                break;
            }
            case IL::OpCode::Or: {
                auto *_or = instr.As<IL::OrInstruction>();

                const Backend::IL::Type* lhsType = ilTypeMap.GetType(_or->lhs);

                SpvOp op;
                switch (lhsType->kind) {
                    default:
                        ASSERT(false, "Invalid And operand type");
                        op = SpvOpBitwiseOr;
                        break;
                    case Backend::IL::TypeKind::Bool:
                        op = SpvOpLogicalOr;
                        break;
                    case Backend::IL::TypeKind::Int:
                        op = SpvOpBitwiseOr;
                        break;
                }

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, _or->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = _or->result;
                spv[3] = idMap.Get(_or->lhs);
                spv[4] = idMap.Get(_or->rhs);
                break;
            }
            case IL::OpCode::And: {
                auto *_and = instr.As<IL::AndInstruction>();

                const Backend::IL::Type* lhsType = ilTypeMap.GetType(_and->lhs);

                SpvOp op;
                switch (lhsType->kind) {
                    default:
                        ASSERT(false, "Invalid And operand type");
                        op = SpvOpBitwiseAnd;
                        break;
                    case Backend::IL::TypeKind::Bool:
                        op = SpvOpLogicalAnd;
                        break;
                    case Backend::IL::TypeKind::Int:
                        op = SpvOpBitwiseAnd;
                        break;
                }

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, _and->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = _and->result;
                spv[3] = idMap.Get(_and->lhs);
                spv[4] = idMap.Get(_and->rhs);
                break;
            }
            case IL::OpCode::Any: {
                auto *any = instr.As<IL::AnyInstruction>();

                const Backend::IL::Type* type = ilTypeMap.GetType(any->value);

                if (type->kind != Backend::IL::TypeKind::Vector) {
                    // Non vector bool types, just set the value directly
                    idMap.Set(any->result, any->value);
                } else {
                    SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpAny, 4, any->source);
                    spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                    spv[2] = any->result;
                    spv[3] = idMap.Get(any->value);
                }
                break;
            }
            case IL::OpCode::All: {
                auto *all = instr.As<IL::AllInstruction>();

                const Backend::IL::Type* type = ilTypeMap.GetType(all->value);

                if (type->kind != Backend::IL::TypeKind::Vector) {
                    // Non vector bool types, just set the value directly
                    idMap.Set(all->result, all->value);
                } else {
                    SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpAll, 4, all->source);
                    spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                    spv[2] = all->result;
                    spv[3] = idMap.Get(all->value);
                }
                break;
            }
            case IL::OpCode::Equal: {
                auto *equal = instr.As<IL::EqualInstruction>();

                const Backend::IL::Type* lhsType = ilTypeMap.GetType(equal->lhs);

                SpvOp op;
                switch (lhsType->kind) {
                    default:
                        ASSERT(false, "Invalid Equal operand type");
                        op = SpvOpIEqual;
                        break;
                    case Backend::IL::TypeKind::Bool:
                        op = SpvOpLogicalEqual;
                        break;
                    case Backend::IL::TypeKind::FP:
                        op = SpvOpFOrdEqual;
                        break;
                    case Backend::IL::TypeKind::Int:
                        op = SpvOpIEqual;
                        break;
                }

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, equal->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = equal->result;
                spv[3] = idMap.Get(equal->lhs);
                spv[4] = idMap.Get(equal->rhs);
                break;
            }
            case IL::OpCode::NotEqual: {
                auto *notEqual = instr.As<IL::NotEqualInstruction>();

                const Backend::IL::Type* lhsType = ilTypeMap.GetType(notEqual->lhs);

                SpvOp op;
                switch (lhsType->kind) {
                    default:
                        ASSERT(false, "Invalid NotEqual operand type");
                        op = SpvOpINotEqual;
                        break;
                    case Backend::IL::TypeKind::Bool:
                        op = SpvOpLogicalNotEqual;
                        break;
                    case Backend::IL::TypeKind::FP:
                        op = SpvOpFOrdNotEqual;
                        break;
                    case Backend::IL::TypeKind::Int:
                        op = SpvOpINotEqual;
                        break;
                }

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, notEqual->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = notEqual->result;
                spv[3] = idMap.Get(notEqual->lhs);
                spv[4] = idMap.Get(notEqual->rhs);
                break;
            }
            case IL::OpCode::LessThan: {
                auto *lessThan = instr.As<IL::LessThanInstruction>();

                const Backend::IL::Type* lhsType = ilTypeMap.GetType(lessThan->lhs);
                const Backend::IL::Type* component = GetComponentType(lhsType);

                SpvOp op;
                if (component->kind == Backend::IL::TypeKind::Int) {
                    op = component->As<Backend::IL::IntType>()->signedness ? SpvOpSLessThan : SpvOpULessThan;
                } else {
                    op = SpvOpFOrdLessThan;
                }

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, lessThan->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = lessThan->result;
                spv[3] = idMap.Get(lessThan->lhs);
                spv[4] = idMap.Get(lessThan->rhs);
                break;
            }
            case IL::OpCode::LessThanEqual: {
                auto *lessThanEqual = instr.As<IL::LessThanEqualInstruction>();

                const Backend::IL::Type* lhsType = ilTypeMap.GetType(lessThanEqual->lhs);
                const Backend::IL::Type* component = GetComponentType(lhsType);

                SpvOp op;
                if (component->kind == Backend::IL::TypeKind::Int) {
                    op = component->As<Backend::IL::IntType>()->signedness ? SpvOpSLessThanEqual : SpvOpULessThanEqual;
                } else {
                    op = SpvOpFOrdLessThanEqual;
                }

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, lessThanEqual->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = lessThanEqual->result;
                spv[3] = idMap.Get(lessThanEqual->lhs);
                spv[4] = idMap.Get(lessThanEqual->rhs);
                break;
            }
            case IL::OpCode::GreaterThan: {
                auto *greaterThan = instr.As<IL::GreaterThanInstruction>();

                const Backend::IL::Type* lhsType = ilTypeMap.GetType(greaterThan->lhs);
                const Backend::IL::Type* component = GetComponentType(lhsType);

                SpvOp op;
                if (component->kind == Backend::IL::TypeKind::Int) {
                    op = component->As<Backend::IL::IntType>()->signedness ? SpvOpSGreaterThan : SpvOpUGreaterThan;
                } else {
                    op = SpvOpFOrdGreaterThan;
                }

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, greaterThan->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = greaterThan->result;
                spv[3] = idMap.Get(greaterThan->lhs);
                spv[4] = idMap.Get(greaterThan->rhs);
                break;
            }
            case IL::OpCode::GreaterThanEqual: {
                auto *greaterThanEqual = instr.As<IL::GreaterThanEqualInstruction>();

                const Backend::IL::Type* lhsType = ilTypeMap.GetType(greaterThanEqual->lhs);
                const Backend::IL::Type* component = GetComponentType(lhsType);

                SpvOp op;
                if (component->kind == Backend::IL::TypeKind::Int) {
                    op = component->As<Backend::IL::IntType>()->signedness ? SpvOpSGreaterThanEqual : SpvOpUGreaterThanEqual;
                } else {
                    op = SpvOpFOrdGreaterThanEqual;
                }

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, greaterThanEqual->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = greaterThanEqual->result;
                spv[3] = idMap.Get(greaterThanEqual->lhs);
                spv[4] = idMap.Get(greaterThanEqual->rhs);
                break;
            }
            case IL::OpCode::IsInf: {
                auto *isInf = instr.As<IL::IsInfInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpIsInf, 4, isInf->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = isInf->result;
                spv[3] = idMap.Get(isInf->value);
                break;
            }
            case IL::OpCode::IsNaN: {
                auto *isNaN = instr.As<IL::IsNaNInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpIsNan, 4, isNaN->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = isNaN->result;
                spv[3] = idMap.Get(isNaN->value);
                break;
            }
            case IL::OpCode::Select: {
                auto *select = instr.As<IL::SelectInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpSelect, 6, select->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = select->result;
                spv[3] = idMap.Get(select->condition);
                spv[4] = idMap.Get(select->pass);
                spv[5] = idMap.Get(select->fail);
                break;
            }
            case IL::OpCode::Branch: {
                auto *branch = instr.As<IL::BranchInstruction>();

                // Write cfg
                if (branch->controlFlow._continue != IL::InvalidID) {
                    SpvInstruction& cfg = stream.Allocate(SpvOpLoopMerge, 4);
                    cfg[1] = branch->controlFlow.merge;
                    cfg[2] = branch->controlFlow._continue;
                    cfg[3] = SpvSelectionControlMaskNone;
                }

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpBranch, 2, branch->source);
                spv[1] = branch->branch;
                break;
            }
            case IL::OpCode::BranchConditional: {
                auto *branch = instr.As<IL::BranchConditionalInstruction>();

                // Write cfg
                if (branch->controlFlow._continue != IL::InvalidID) {
                    SpvInstruction& cfg = stream.Allocate(SpvOpLoopMerge, 4);
                    cfg[1] = branch->controlFlow.merge;
                    cfg[2] = branch->controlFlow._continue;
                    cfg[3] = SpvSelectionControlMaskNone;
                } else if (branch->controlFlow.merge != IL::InvalidID) {
                    SpvInstruction& cfg = stream.Allocate(SpvOpSelectionMerge, 3);
                    cfg[1] = branch->controlFlow.merge;
                    cfg[2] = SpvSelectionControlMaskNone;
                }

                // Perform the branch, must be after cfg instruction
                SpvInstruction& spv = stream.Allocate(SpvOpBranchConditional, 4);
                spv[1] = idMap.Get(branch->cond);
                spv[2] = branch->pass;
                spv[3] = branch->fail;
                break;
            }
            case IL::OpCode::Switch: {
                auto *_switch = instr.As<IL::SwitchInstruction>();

                // Write cfg
                if (_switch->controlFlow.merge != IL::InvalidID) {
                    SpvInstruction& cfg = stream.Allocate(SpvOpSelectionMerge, 3);
                    cfg[1] = _switch->controlFlow.merge;
                    cfg[2] = SpvSelectionControlMaskNone;
                }

                // Perform the switch, must be after cfg instruction
                SpvInstruction& spv = stream.Allocate(SpvOpSwitch, 3 + 2 * _switch->cases.count);
                spv[1] = idMap.Get(_switch->value);
                spv[2] = _switch->_default;

                for (uint32_t i = 0; i < _switch->cases.count; i++) {
                    const IL::SwitchCase& _case = _switch->cases[i];
                    spv[3 + i * 2] = _case.literal;
                    spv[4 + i * 2] = _case.branch;
                }
                break;
            }
            case IL::OpCode::Phi: {
                auto *phi = instr.As<IL::PhiInstruction>();

                SpvInstruction& spv = stream.Allocate(SpvOpPhi, 3 + 2 * phi->values.count);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = phi->result;

                for (uint32_t i = 0; i < phi->values.count; i++) {
                    const IL::PhiValue& value = phi->values[i];
                    spv[3 + i * 2] = value.value;
                    spv[4 + i * 2] = value.branch;
                }
                break;
            }
            case IL::OpCode::BitCast: {
                auto *bitCast = instr.As<IL::BitCastInstruction>();

                // Get value type
                const Backend::IL::Type* valueType = ilTypeMap.GetType(bitCast->value);

                // Any need to cast at all?
                if (valueType == resultType) {
                    // Same, just set the value directly
                    idMap.Set(bitCast->result, bitCast->value);
                } else {
                    SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpBitcast, 4, bitCast->source);
                    spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                    spv[2] = bitCast->result;
                    spv[3] = bitCast->value;
                }
                break;
            }
            case IL::OpCode::BitOr: {
                auto *bitOr = instr.As<IL::BitOrInstruction>();

                SpvOp op;
                if (ilTypeMap.GetType(bitOr->lhs)->Is<Backend::IL::BoolType>()) {
                    op = SpvOpLogicalOr;
                } else {
                    op = SpvOpBitwiseOr;
                }

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, bitOr->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = bitOr->result;
                spv[3] = idMap.Get(bitOr->lhs);
                spv[4] = idMap.Get(bitOr->rhs);
                break;
            }
            case IL::OpCode::BitAnd: {
                auto *bitAnd = instr.As<IL::BitAndInstruction>();

                SpvOp op;
                if (ilTypeMap.GetType(bitAnd->lhs)->Is<Backend::IL::BoolType>()) {
                    op = SpvOpLogicalAnd;
                } else {
                    op = SpvOpBitwiseAnd;
                }

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, bitAnd->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = bitAnd->result;
                spv[3] = idMap.Get(bitAnd->lhs);
                spv[4] = idMap.Get(bitAnd->rhs);
                break;
            }
            case IL::OpCode::BitShiftLeft: {
                auto *bsl = instr.As<IL::BitShiftLeftInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpShiftLeftLogical, 5, bsl->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = bsl->result;
                spv[3] = idMap.Get(bsl->value);
                spv[4] = idMap.Get(bsl->shift);
                break;
            }
            case IL::OpCode::BitShiftRight: {
                auto *bsr = instr.As<IL::BitShiftRightInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpShiftRightLogical, 5, bsr->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = bsr->result;
                spv[3] = idMap.Get(bsr->value);
                spv[4] = idMap.Get(bsr->shift);
                break;
            }
            case IL::OpCode::Export: {
                auto *_export = instr.As<IL::ExportInstruction>();
                table.shaderExport.Export(stream, _export->exportID, idMap.Get(_export->value));
                break;
            }
            case IL::OpCode::ResourceToken: {
                auto *token = instr.As<IL::ResourceTokenInstruction>();
                table.shaderPRMT.GetToken(stream, idMap.Get(token->resource), token->result);
                break;
            }
            case IL::OpCode::Alloca: {
                auto *bsr = instr.As<IL::BitShiftRightInstruction>();

                SpvInstruction& spv = table.typeConstantVariable.block->stream.TemplateOrAllocate(SpvOpVariable, 4, bsr->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = bsr->result;
                spv[3] = SpvStorageClassFunction;
                break;
            }
            case IL::OpCode::Load: {
                auto *load = instr.As<IL::LoadInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpLoad, 4, load->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = load->result;
                spv[3] = idMap.Get(load->address);
                break;
            }
            case IL::OpCode::Store: {
                auto *store = instr.As<IL::StoreInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpStore, 3, store->source);
                spv[1] = idMap.Get(store->address);
                spv[2] = idMap.Get(store->value);
                break;
            }
            case IL::OpCode::Extract: {
                auto *extract = instr.As<IL::ExtractInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpCompositeExtract, 5, extract->source);
                spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                spv[2] = idMap.Get(extract->result);
                spv[3] = idMap.Get(extract->composite);
                spv[4] = extract->index;
                break;
            }
            case IL::OpCode::LoadBuffer: {
                auto *loadBuffer = instr.As<IL::LoadBufferInstruction>();

                // Get the buffer type
                const auto* bufferType = ilTypeMap.GetType(loadBuffer->buffer)->As<Backend::IL::BufferType>();

                // Texel buffer?
                if (bufferType->texelType != Backend::IL::Format::None) {
                    // Load image with appropriate instruction
                    if (bufferType->samplerMode == Backend::IL::ResourceSamplerMode::Writable) {
                        SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpImageRead, 5, instr->source);
                        spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                        spv[2] = loadBuffer->result;
                        spv[3] = idMap.Get(loadBuffer->buffer);
                        spv[4] = idMap.Get(loadBuffer->index);
                    } else {
                        SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpImageFetch, 5, instr->source);
                        spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                        spv[2] = loadBuffer->result;
                        spv[3] = idMap.Get(loadBuffer->buffer);
                        spv[4] = idMap.Get(loadBuffer->index);
                    }
                } else {
                    ASSERT(false, "Not implemented");
                }
                break;
            }
            case IL::OpCode::StoreBuffer: {
                auto *storeBuffer = instr.As<IL::StoreBufferInstruction>();

                // Get the buffer type
                const auto* bufferType = ilTypeMap.GetType(storeBuffer->buffer)->As<Backend::IL::BufferType>();

                // Texel buffer?
                if (bufferType->texelType != Backend::IL::Format::None) {
                    // Write image
                    SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpImageWrite, 4, instr->source);
                    spv[1] = idMap.Get(storeBuffer->buffer);
                    spv[2] = idMap.Get(storeBuffer->index);
                    spv[3] = idMap.Get(storeBuffer->value);
                } else {
                    ASSERT(false, "Not implemented");
                    return false;
                }
                break;
            }
            case IL::OpCode::ResourceSize: {
                auto *size = instr.As<IL::ResourceSizeInstruction>();

                // Capability set
                table.capability.Add(SpvCapabilityImageQuery);

                // Get the resource type
                const auto* resourceType = ilTypeMap.GetType(size->resource);

                switch (resourceType->kind) {
                    default:{
                        ASSERT(false, "Invalid ResourceSize type kind");
                        return false;
                    }
                    case Backend::IL::TypeKind::Texture: {
                        auto* texture = resourceType->As<Backend::IL::TextureType>();

                        if (texture->samplerMode == Backend::IL::ResourceSamplerMode::Compatible) {
                            uint32_t constantZeroId = table.scan.header.bound++;

                            // UInt32
                            const Backend::IL::Type *intType = ilTypeMap.FindTypeOrAdd(Backend::IL::IntType{
                                .bitWidth = 32,
                                .signedness = false
                            });

                            SpvInstruction& spvLod = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
                            spvLod[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(intType);
                            spvLod[2] = constantZeroId;
                            spvLod[3] = 0;

                            // Query lod image size
                            SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpImageQuerySizeLod, 5, instr->source);
                            spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                            spv[2] = size->result;
                            spv[3] = idMap.Get(size->resource);
                            spv[4] = constantZeroId;
                        } else {
                            // Query non-lod image size
                            SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpImageQuerySize, 4, instr->source);
                            spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                            spv[2] = size->result;
                            spv[3] = idMap.Get(size->resource);
                        }

                        break;
                    }
                    case Backend::IL::TypeKind::Buffer: {
                        // Texel buffer?
                        if (resourceType->As<Backend::IL::BufferType>()->texelType != Backend::IL::Format::None) {
                            // Query image
                            SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpImageQuerySize, 4, instr->source);
                            spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                            spv[2] = size->result;
                            spv[3] = idMap.Get(size->resource);
                        } else {
                            ASSERT(false, "Not implemented");
                        }
                        break;
                    }
                }

                break;
            }
            case IL::OpCode::AtomicOr:
            case IL::OpCode::AtomicXOr:
            case IL::OpCode::AtomicAnd:
            case IL::OpCode::AtomicAdd:
            case IL::OpCode::AtomicMin:
            case IL::OpCode::AtomicMax:
            case IL::OpCode::AtomicExchange:
            case IL::OpCode::AtomicCompareExchange: {
                // uint32_t
                const Backend::IL::Type* uintType = ilTypeMap.FindTypeOrAdd(Backend::IL::IntType {.bitWidth = 32, .signedness=false});

                // Identifiers
                uint32_t scopeId = table.scan.header.bound++;
                uint32_t memSemanticId = table.scan.header.bound++;

                // Device scope
                SpvInstruction &spvScope = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
                spvScope[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
                spvScope[2] = scopeId;
                spvScope[3] = SpvScopeDevice;

                // No memory mask
                SpvInstruction &spvMemSem = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
                spvMemSem[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(uintType);
                spvMemSem[2] = memSemanticId;
                spvMemSem[3] = SpvMemorySemanticsMaskNone;

                // Handle op code
                switch (instr->opCode) {
                    default:
                        ASSERT(false, "Invalid op code");
                        break;
                    case IL::OpCode::AtomicOr: {
                        auto *_instr = instr.As<IL::AtomicOrInstruction>();

                        SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpAtomicOr, 7, _instr->source);
                        spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                        spv[2] = _instr->result;
                        spv[3] = idMap.Get(_instr->address);
                        spv[4] = scopeId;
                        spv[5] = memSemanticId;
                        spv[6] = idMap.Get(_instr->value);
                        break;
                    }
                    case IL::OpCode::AtomicXOr: {
                        auto *_instr = instr.As<IL::AtomicXOrInstruction>();

                        SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpAtomicXor, 7, _instr->source);
                        spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                        spv[2] = _instr->result;
                        spv[3] = idMap.Get(_instr->address);
                        spv[4] = scopeId;
                        spv[5] = memSemanticId;
                        spv[6] = idMap.Get(_instr->value);
                        break;
                    }
                    case IL::OpCode::AtomicAnd: {
                        auto *_instr = instr.As<IL::AtomicAndInstruction>();

                        SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpAtomicAnd, 7, _instr->source);
                        spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                        spv[2] = _instr->result;
                        spv[3] = idMap.Get(_instr->address);
                        spv[4] = scopeId;
                        spv[5] = memSemanticId;
                        spv[6] = idMap.Get(_instr->value);
                        break;
                    }
                    case IL::OpCode::AtomicAdd: {
                        auto *_instr = instr.As<IL::AtomicAddInstruction>();

                        ASSERT(resultType->kind == Backend::IL::TypeKind::Int, "Only integral atomics are supported for recompilation");

                        SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpAtomicIAdd, 7, _instr->source);
                        spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                        spv[2] = _instr->result;
                        spv[3] = idMap.Get(_instr->address);
                        spv[4] = scopeId;
                        spv[5] = memSemanticId;
                        spv[6] = idMap.Get(_instr->value);
                        break;
                    }
                    case IL::OpCode::AtomicMin: {
                        auto *_instr = instr.As<IL::AtomicMinInstruction>();

                        ASSERT(resultType->kind == Backend::IL::TypeKind::Int, "Only integral atomics are supported for recompilation");
                        SpvOp op = resultType->As<Backend::IL::IntType>()->signedness ? SpvOpAtomicSMin : SpvOpAtomicUMin;

                        SpvInstruction& spv = stream.TemplateOrAllocate(op, 7, _instr->source);
                        spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                        spv[2] = _instr->result;
                        spv[3] = idMap.Get(_instr->address);
                        spv[4] = scopeId;
                        spv[5] = memSemanticId;
                        spv[6] = idMap.Get(_instr->value);
                        break;
                    }
                    case IL::OpCode::AtomicMax: {
                        auto *_instr = instr.As<IL::AtomicMaxInstruction>();

                        ASSERT(resultType->kind == Backend::IL::TypeKind::Int, "Only integral atomics are supported for recompilation");
                        SpvOp op = resultType->As<Backend::IL::IntType>()->signedness ? SpvOpAtomicSMax : SpvOpAtomicUMax;

                        SpvInstruction& spv = stream.TemplateOrAllocate(op, 7, _instr->source);
                        spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                        spv[2] = _instr->result;
                        spv[3] = idMap.Get(_instr->address);
                        spv[4] = scopeId;
                        spv[5] = memSemanticId;
                        spv[6] = idMap.Get(_instr->value);
                        break;
                    }
                    case IL::OpCode::AtomicExchange: {
                        auto *_instr = instr.As<IL::AtomicExchangeInstruction>();

                        SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpAtomicExchange, 7, _instr->source);
                        spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                        spv[2] = _instr->result;
                        spv[3] = idMap.Get(_instr->address);
                        spv[4] = scopeId;
                        spv[5] = memSemanticId;
                        spv[6] = idMap.Get(_instr->value);
                        break;
                    }
                    case IL::OpCode::AtomicCompareExchange: {
                        auto *_instr = instr.As<IL::AtomicCompareExchangeInstruction>();

                        SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpAtomicCompareExchange, 9, _instr->source);
                        spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                        spv[2] = _instr->result;
                        spv[3] = idMap.Get(_instr->address);
                        spv[4] = scopeId;
                        spv[5] = memSemanticId;
                        spv[6] = memSemanticId;
                        spv[7] = idMap.Get(_instr->value);
                        spv[8] = idMap.Get(_instr->comparator);
                        break;
                    }
                }
                break;
            }
            case IL::OpCode::AddressChain: {
                auto *_instr = instr.As<IL::AddressChainInstruction>();

                // Get resulting type
                const auto* pointerType = resultType->As<Backend::IL::PointerType>();

                // Texel addresses must be handled separately
                if (pointerType->addressSpace == Backend::IL::AddressSpace::Texture || pointerType->addressSpace == Backend::IL::AddressSpace::Buffer) {
                    ASSERT(_instr->chains.count == 1, "Resource address chains do not support a depth greater than 1");

                    // Id allocations
                    uint32_t spvMSId = table.scan.header.bound++;

                    // UInt32
                    const Backend::IL::Type *intType = ilTypeMap.FindTypeOrAdd(Backend::IL::IntType{
                        .bitWidth = 32,
                        .signedness = false
                    });

                    // No MS
                    SpvInstruction& spvMSSpv = table.typeConstantVariable.block->stream.Allocate(SpvOpConstant, 4);
                    spvMSSpv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(intType);
                    spvMSSpv[2] = spvMSId;
                    spvMSSpv[3] = 0;

                    SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpImageTexelPointer, 6, _instr->source);
                    spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                    spv[2] = _instr->result;
                    spv[3] = idMap.Get(_instr->composite);
                    spv[4] = idMap.Get(_instr->chains[0].index);
                    spv[5] = spvMSId;
                } else {
                    SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpAccessChain, 4 + _instr->chains.count, _instr->source);
                    spv[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(resultType);
                    spv[2] = _instr->result;
                    spv[3] = idMap.Get(_instr->composite);

                    // Write chains (accessors from base composite value)
                    for (uint32_t i = 0; i < _instr->chains.count; i++) {
                        spv[4 + i] = idMap.Get(_instr->chains[i].index);
                    }
                }
                break;
            }
        }
    }

    // OK
    return true;
}

bool SpvPhysicalBlockFunction::PostPatchLoopContinueInstruction(IL::Instruction *instruction, IL::ID original, IL::ID redirect) {
    switch (instruction->opCode) {
        default:
            return false;
        case IL::OpCode::Branch: {
            auto* branch = instruction->As<IL::BranchInstruction>();
            branch->branch = redirect;
            return true;
        }
        case IL::OpCode::BranchConditional: {
            auto* branch = instruction->As<IL::BranchConditionalInstruction>();

            // Pass branch
            if (branch->pass == original) {
                branch->pass = redirect;
                return true;
            }

            // Fail branch
            if (branch->fail == original) {
                branch->fail = redirect;
                return true;
            }

            // No need
            return false;
        }
    }
}

void SpvPhysicalBlockFunction::PostPatchLoopContinue(IL::Function* fn) {
    for (const LoopContinueBlock& block : loopContinueBlocks) {
        IL::ID bridgeBlockId = program.GetIdentifierMap().AllocID();

        // All removed users
        TrivialStackVector<IL::OpaqueInstructionRef, 128> removed;

        // Change all users
        for (const IL::OpaqueInstructionRef& ref : program.GetIdentifierMap().GetBlockUsers(block.block)) {
            IL::Instruction* instruction = ref.basicBlock->GetRelocationInstruction(ref.relocationOffset);

            // Attempt to patch the instruction
            if (!PostPatchLoopContinueInstruction(instruction, block.block, bridgeBlockId)) {
                continue;
            }

            // Add new block user
            program.GetIdentifierMap().AddBlockUser(bridgeBlockId, ref);

            // Mark user instruction as dirty
            instruction->source = instruction->source.Modify();

            // Mark the branch block as dirty to ensure recompilation
            ref.basicBlock->MarkAsDirty();

            // Latent removal
            removed.Add(ref);
        }

        // Remove references
        for (const IL::OpaqueInstructionRef& ref : removed) {
            program.GetIdentifierMap().RemoveBlockUser(block.block, ref);
        }

        // Get the original continue block, set new id
        IL::BasicBlock* originalContinueBlock = fn->GetBasicBlocks().GetBlock(block.block);
        originalContinueBlock->SetID(bridgeBlockId);

        // Allocate continue proxy block, use the same id
        IL::BasicBlock* postContinueBlock = fn->GetBasicBlocks().AllocBlock(block.block);

        // Modify original block
        {
            // Get the terminator
            auto it = originalContinueBlock->GetTerminator();
            ASSERT(it->Is<IL::BranchInstruction>(), "Unexpected terminator");
            ASSERT(it->As<IL::BranchInstruction>()->branch == block.instruction.basicBlock->GetID(), "Unexpected loop terminator block");

            // Remove the original branch instruction (back to the header)
            originalContinueBlock->Remove(it);

            // Branch to the post block
            IL::Emitter<> emitter(program, *originalContinueBlock);
            emitter.Branch(postContinueBlock);
        }

        // Modify post block
        {
            // Never instrument this block
            postContinueBlock->AddFlag(BasicBlockFlag::NoInstrumentation);

            // Branch back to the loop header
            IL::Emitter<> emitter(program, *postContinueBlock);
            emitter.Branch(block.instruction.basicBlock);
        }
    }

    // Empty out
    loopContinueBlocks.clear();
}

void SpvPhysicalBlockFunction::CreateDataResourceMap(const SpvJob& job) {
    // Get data map
    IL::ShaderDataMap& shaderDataMap = program.GetShaderDataMap();

    // Get IL map
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();

    // Current offset
    uint32_t shaderDataOffset = 0;

    // Emit all resources
    for (const ShaderDataInfo& info : shaderDataMap) {
        if (!(info.type & ShaderDataType::DescriptorMask)) {
            continue;
        }

        // Get variable
        const Backend::IL::Variable* variable = shaderDataMap.Get(info.id);

        // Variables always pointer to
        const auto* pointerType = variable->type->As<Backend::IL::PointerType>();

        // Only buffers supported for now
        ASSERT(info.type == ShaderDataType::Buffer, "Only buffers are implemented for now");

        // RWBuffer<uint>*
        auto* bufferPtrType = ilTypeMap.FindTypeOrAdd(Backend::IL::PointerType{
            .pointee =  pointerType->pointee->As<Backend::IL::BufferType>(),
            .addressSpace = Backend::IL::AddressSpace::Resource,
        });

        // SpvIds
        SpvId bufferPtrTypeId = table.typeConstantVariable.typeMap.GetSpvTypeId(bufferPtrType);

        // Counter
        SpvInstruction &spvCounterVar = table.typeConstantVariable.block->stream.Allocate(SpvOpVariable, 4);
        spvCounterVar[1] = bufferPtrTypeId;
        spvCounterVar[2] = variable->id;
        spvCounterVar[3] = SpvStorageClassUniformConstant;

        // Descriptor set
        SpvInstruction &spvCounterSet = table.annotation.block->stream.Allocate(SpvOpDecorate, 4);
        spvCounterSet[1] = variable->id;
        spvCounterSet[2] = SpvDecorationDescriptorSet;
        spvCounterSet[3] = job.instrumentationKey.pipelineLayoutUserSlots;

        // Binding
        SpvInstruction &spvCounterBinding = table.annotation.block->stream.Allocate(SpvOpDecorate, 4);
        spvCounterBinding[1] = variable->id;
        spvCounterBinding[2] = SpvDecorationBinding;
        spvCounterBinding[3] = job.bindingInfo.shaderDataDescriptorOffset + shaderDataOffset;
    }
}

void SpvPhysicalBlockFunction::CreateDataPCMap(const SpvJob &job) {
    // Get data map
    IL::ShaderDataMap& shaderDataMap = program.GetShaderDataMap();

    // Get IL map
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();

    // Requested dword count
    uint32_t dwordCount = 0;

    // Aggregate dword count
    for (const ShaderDataInfo& info : shaderDataMap) {
        if (info.type == ShaderDataType::Event) {
            dwordCount++;
        }
    }

    // Early out if no requests
    if (!dwordCount) {
        return;
    }

    // UInt32
    const Backend::IL::Type *intType = ilTypeMap.FindTypeOrAdd(Backend::IL::IntType{
        .bitWidth = 32,
        .signedness = false
    });

    // One dword per event, emit as individual members
    Backend::IL::StructType structDecl;
    for (uint32_t i = 0; i < dwordCount; i++) {
        structDecl.memberTypes.push_back(intType);
    }

    // Create type
    pcBlockType = ilTypeMap.FindTypeOrAdd(structDecl);

    // Pointer to block
    auto* bufferPtrType = ilTypeMap.FindTypeOrAdd(Backend::IL::PointerType{
        .pointee = pcBlockType,
        .addressSpace = Backend::IL::AddressSpace::RootConstant
    });

    // SpvIds
    SpvId pcBlockTypeId = table.typeConstantVariable.typeMap.GetSpvTypeId(pcBlockType);
    SpvId bufferPtrTypeId = table.typeConstantVariable.typeMap.GetSpvTypeId(bufferPtrType);

    // Id allocations
    pcBlockId = table.scan.header.bound++;

    // Declare push constant block
    SpvInstruction &spvCounterVar = table.typeConstantVariable.block->stream.Allocate(SpvOpVariable, 4);
    spvCounterVar[1] = bufferPtrTypeId;
    spvCounterVar[2] = pcBlockId;
    spvCounterVar[3] = SpvStorageClassPushConstant;

    // Block annotation
    SpvInstruction &pcBlock = table.annotation.block->stream.Allocate(SpvOpDecorate, 3);
    pcBlock[1] = pcBlockTypeId;
    pcBlock[2] = SpvDecorationBlock;

    // Block annotation
    for (uint32_t i = 0; i < dwordCount; i++) {
        SpvInstruction &pcBlockMember = table.annotation.block->stream.Allocate(SpvOpMemberDecorate, 5);
        pcBlockMember[1] = pcBlockTypeId;
        pcBlockMember[2] = i;
        pcBlockMember[3] = SpvDecorationOffset;
        pcBlockMember[4] = sizeof(uint32_t) * i;
    }
}

void SpvPhysicalBlockFunction::CreateDataLookups(SpvStream& stream, SpvIdMap& idMap) {
    if (!pcBlockType) {
        return;
    }

    // Get data map
    IL::ShaderDataMap& shaderDataMap = program.GetShaderDataMap();

    // Get IL map
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();

    // UInt32
    const Backend::IL::Type *intType = ilTypeMap.FindTypeOrAdd(Backend::IL::IntType{
        .bitWidth = 32,
        .signedness = false
    });

    // Id allocations
    uint32_t pcBlockLoadId = table.scan.header.bound++;

    // Load pc block
    SpvInstruction& spvLoad = stream.Allocate(SpvOpLoad, 4);
    spvLoad[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(pcBlockType);
    spvLoad[2] = pcBlockLoadId;
    spvLoad[3] = pcBlockId;

    // Current dword offset
    uint32_t dwordOffset = 0;

    // Aggregate dword count
    for (const ShaderDataInfo& info : shaderDataMap) {
        if (info.type != ShaderDataType::Event) {
            continue;
        }

        // Get variable
        const Backend::IL::Variable* variable = shaderDataMap.Get(info.id);

        // Id allocations
        IL::ID pcRedirect = table.scan.header.bound++;

        // Fetch texel
        SpvInstruction& spvExtract = stream.Allocate(SpvOpCompositeExtract, 5);
        spvExtract[1] = table.typeConstantVariable.typeMap.GetSpvTypeId(intType);
        spvExtract[2] = pcRedirect;
        spvExtract[3] = pcBlockLoadId;
        spvExtract[4] = dwordOffset;

        // Set the identifier redirect, the frontend exposes the event ids as constant IDs independent of the function.
        // However, as multiple functions can be instrumented we have to load them per function, use the redirector in this case.
        idMap.Set(variable->id, pcRedirect);

        // Next!
        dwordOffset++;
    }
}

void SpvPhysicalBlockFunction::CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockFunction &out) {
    out.block = remote.scan.GetPhysicalBlock(SpvPhysicalBlockType::Function);
}
