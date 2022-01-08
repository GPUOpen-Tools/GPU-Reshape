#include <Backends/Vulkan/Compiler/SpvModule.h>
#include <Backends/Vulkan/Compiler/SpvInstruction.h>
#include <Backends/Vulkan/Compiler/SpvStream.h>
#include <Backends/Vulkan/Compiler/SpvRelocationStream.h>

void SpvModule::AllocateSectionBlocks(SpvRelocationStream& stream) {
    // Get the header
    stream.AllocateFixedBlock(IL::SourceSpan{0, IL::WordCount<ProgramHeader>()}, &header);

    // Set the new number of bound values
    header.bound = program->GetIdentifierMap().GetMaxID();

    // Allocate the blocks
    for (uint32_t i = 0; i < static_cast<uint32_t>(LayoutSectionType::Count); i++) {
        metaData.sections[i].stream = &stream.AllocateBlock(metaData.sections[i].sourceSpan.AppendSpan()).stream;
    }

    // Set insertion stream
    typeMap->SetDeclarationStream(GetSection(LayoutSectionType::Declarations).stream);

    // Set the type counter
    typeMap->SetIdCounter(&header.bound);
}

bool SpvModule::Recompile(const uint32_t *code, uint32_t wordCount, const SpvJob& job) {
    // Compilation relocation stream
    SpvRelocationStream relocationStream(code, wordCount);

    // Allocate all blocks
    AllocateSectionBlocks(relocationStream);

    // Inject the export records
    InsertExportRecords(relocationStream, job);

    // Go through all functions
    for (auto fn = program->begin(); fn != program->end(); fn++) {
        if (!RecompileFunction(relocationStream, *fn)) {
            return false;
        }
    }

    // Stitch the final program
    spirvProgram.clear();
    relocationStream.Stitch(spirvProgram);

    // OK!
    return true;
}

void SpvModule::InsertExportRecords(SpvRelocationStream &stream, const SpvJob& job) {
    // Note: This is quite ugly, will be changed

    // IL type map
    Backend::IL::TypeMap& ilTypeMap = program->GetTypeMap();

    // Allocate new blocks
    SpvStream& annotations = *GetSection(LayoutSectionType::Annotation).stream;
    SpvStream& declarations = *GetSection(LayoutSectionType::Declarations).stream;

    // UInt32
    const Backend::IL::Type* intType = ilTypeMap.FindTypeOrAdd(Backend::IL::IntType {
        .bitWidth = 32,
        .signedness = false
    });

    // RWBuffer<uint>
    metaData.exportMd.buffer32UIRW = ilTypeMap.FindTypeOrAdd(Backend::IL::BufferType {
        .elementType = intType,
        .texelType = Backend::IL::Format::R32UInt
    });

    // RWBuffer<uint>*
    metaData.exportMd.buffer32UIRWPtr = ilTypeMap.FindTypeOrAdd(Backend::IL::PointerType {
        .pointee = metaData.exportMd.buffer32UIRW,
        .addressSpace = Backend::IL::AddressSpace::Resource,
    });

    // RWBuffer<uint>[N]
    const Backend::IL::Type* buffer32UIWWArray = ilTypeMap.FindTypeOrAdd(Backend::IL::ArrayType {
        .elementType = metaData.exportMd.buffer32UIRW,
        .count = std::max(1u, job.streamCount)
    });

    // RWBuffer<uint>[N]*
    metaData.exportMd.buffer32UIRWArrayPtr = ilTypeMap.FindTypeOrAdd(Backend::IL::PointerType {
        .pointee = buffer32UIWWArray,
        .addressSpace = Backend::IL::AddressSpace::Resource,
    });

    // Id allocations
    metaData.exportMd.counterId = header.bound++;
    metaData.exportMd.streamId = header.bound++;

    // SpvIds
    SpvId buffer32UIRWPtrId = typeMap->GetSpvTypeId(metaData.exportMd.buffer32UIRWPtr);
    SpvId buffer32UIRWArrayPtrId = typeMap->GetSpvTypeId(metaData.exportMd.buffer32UIRWArrayPtr);

    // Counter
    SpvInstruction& spvCounterVar = declarations.Allocate(SpvOpVariable, 4);
    spvCounterVar[1] = buffer32UIRWPtrId;
    spvCounterVar[2] = metaData.exportMd.counterId;
    spvCounterVar[3] = SpvStorageClassUniformConstant;

    // Streams
    SpvInstruction& spvStreamVar = declarations.Allocate(SpvOpVariable, 4);
    spvStreamVar[1] = buffer32UIRWArrayPtrId;
    spvStreamVar[2] = metaData.exportMd.streamId;
    spvStreamVar[3] = SpvStorageClassUniformConstant;

    // Descriptor set
    SpvInstruction& spvCounterSet = annotations.Allocate(SpvOpDecorate, 4);
    spvCounterSet[1] = metaData.exportMd.counterId;
    spvCounterSet[2] = SpvDecorationDescriptorSet;
    spvCounterSet[3] = job.instrumentationKey.pipelineLayoutUserSlots;

    // Binding
    SpvInstruction& spvCounterBinding = annotations.Allocate(SpvOpDecorate, 4);
    spvCounterBinding[1] = metaData.exportMd.counterId;
    spvCounterBinding[2] = SpvDecorationBinding;
    spvCounterBinding[3] = 0;

    // Descriptor set
    SpvInstruction& spvStreamSet = annotations.Allocate(SpvOpDecorate, 4);
    spvStreamSet[1] = metaData.exportMd.streamId;
    spvStreamSet[2] = SpvDecorationDescriptorSet;
    spvStreamSet[3] = job.instrumentationKey.pipelineLayoutUserSlots;

    // Binding
    SpvInstruction& spvStreamBinding = annotations.Allocate(SpvOpDecorate, 4);
    spvStreamBinding[1] = metaData.exportMd.streamId;
    spvStreamBinding[2] = SpvDecorationBinding;
    spvStreamBinding[3] = 1;
}

bool SpvModule::RecompileFunction(SpvRelocationStream& relocationStream, IL::Function& fn) {
    // Find all dirty basic blocks
    for (auto bb = fn.begin(); bb != fn.end(); bb++) {
        // If not modified, skip
        if (!bb->IsModified()) {
            continue;
        }

        // Attempt to recompile the block
        if (!RecompileBasicBlock(relocationStream, fn, bb)) {
            return false;
        }
    }

    // OK
    return true;
}

bool SpvModule::RecompileBasicBlock(SpvRelocationStream& relocationStream, IL::Function& fn, std::list<IL::BasicBlock>::iterator bb) {
    // IL type map
    Backend::IL::TypeMap& ilTypeMap = program->GetTypeMap();

    // Declaration stream
    SpvStream* declarationStream = GetSection(LayoutSectionType::Declarations).stream;

    // Get the source span
    IL::SourceSpan span = bb->GetSourceSpan();

    // Foreign block?
    if (span.begin == IL::InvalidOffset) {
        // TODO: What if it's the first, what then?
        auto previous = std::prev(bb);

        // Allocate foreign block right after previous
        span.begin = previous->GetSourceSpan().end;
        span.end = previous->GetSourceSpan().end;

        // Assign for successors
        bb->SetSourceSpan(span);
    }

    // Create a new relocation block
    SpvRelocationBlock &relocationBlock = relocationStream.AllocateBlock(span);

    // Destination stream
    SpvStream &stream = relocationBlock.stream;

    // Emit label
    SpvInstruction& label = stream.Allocate(SpvOpLabel, 2);
    label[1] = bb->GetID();

    // Emit all backend instructions
    for (auto instr = bb->begin(); instr != bb->end(); instr++) {
        // If trivial, just copy it directly
        if (instr->source.TriviallyCopyable()) {
            auto ptr = instr.Get();
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

                SpvInstruction& spv = declarationStream->Allocate(SpvOpConstant, 4);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = literal->result;
                spv[3] = literal->value.integral;
                break;
            }
            case IL::OpCode::LoadTexture: {
                auto *loadTexture = instr.As<IL::LoadTextureInstruction>();

                // Get the texture type
                const auto* textureType = ilTypeMap.GetType(loadTexture->texture)->As<Backend::IL::TextureType>();

                // Load image
                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpImageRead, 5, instr->source);
                spv[1] = typeMap->GetSpvTypeId(textureType->sampledType);
                spv[2] = loadTexture->result;
                spv[3] = loadTexture->texture;
                spv[4] = loadTexture->index;
                break;
            }
            case IL::OpCode::StoreTexture: {
                auto *storeTexture = instr.As<IL::StoreTextureInstruction>();

                // Get the texture type
                const auto* textureType = ilTypeMap.GetType(storeTexture->texture)->As<Backend::IL::TextureType>();

                // Write image
                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpImageWrite, 4, instr->source);
                spv[1] = storeTexture->texture;
                spv[2] = storeTexture->index;
                spv[3] = storeTexture->texel;
                break;
            }
            case IL::OpCode::Add: {
                auto *add = instr.As<IL::AddInstruction>();

                SpvOp op = resultType->kind == Backend::IL::TypeKind::FP ? SpvOpFAdd : SpvOpIAdd;

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, add->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = add->result;
                spv[3] = add->lhs;
                spv[4] = add->rhs;
                break;
            }
            case IL::OpCode::Sub: {
                auto *add = instr.As<IL::SubInstruction>();

                SpvOp op = resultType->kind == Backend::IL::TypeKind::FP ? SpvOpFSub : SpvOpISub;

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, add->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = add->result;
                spv[3] = add->lhs;
                spv[4] = add->rhs;
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
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = add->result;
                spv[3] = add->lhs;
                spv[4] = add->rhs;
                break;
            }
            case IL::OpCode::Mul: {
                auto *add = instr.As<IL::MulInstruction>();

                SpvOp op = resultType->kind == Backend::IL::TypeKind::FP ? SpvOpFMul : SpvOpIMul;

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, add->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = add->result;
                spv[3] = add->lhs;
                spv[4] = add->rhs;
                break;
            }
            case IL::OpCode::Or: {
                auto *_or = instr.As<IL::OrInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpLogicalOr, 5, _or->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = _or->result;
                spv[3] = _or->lhs;
                spv[4] = _or->rhs;
                break;
            }
            case IL::OpCode::And: {
                auto *_and = instr.As<IL::AndInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpLogicalAnd, 5, _and->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = _and->result;
                spv[3] = _and->lhs;
                spv[4] = _and->rhs;
                break;
            }
            case IL::OpCode::Any: {
                auto *any = instr.As<IL::AnyInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpAny, 4, any->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = any->result;
                spv[3] = any->value;
                break;
            }
            case IL::OpCode::All: {
                auto *all = instr.As<IL::AllInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpAll, 4, all->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = all->result;
                spv[3] = all->value;
                break;
            }
            case IL::OpCode::Equal: {
                auto *equal = instr.As<IL::EqualInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpLogicalEqual, 5, equal->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = equal->result;
                spv[3] = equal->lhs;
                spv[4] = equal->rhs;
                break;
            }
            case IL::OpCode::NotEqual: {
                auto *notEqual = instr.As<IL::NotEqualInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpLogicalNotEqual, 5, notEqual->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = notEqual->result;
                spv[3] = notEqual->lhs;
                spv[4] = notEqual->rhs;
                break;
            }
            case IL::OpCode::LessThan: {
                auto *lessThan = instr.As<IL::LessThanInstruction>();

                const Backend::IL::Type* lhsType = ilTypeMap.GetType(lessThan->lhs);

                SpvOp op;
                if (lhsType->kind == Backend::IL::TypeKind::Int) {
                    op = lhsType->As<Backend::IL::IntType>()->signedness ? SpvOpSLessThan : SpvOpULessThan;
                } else {
                    op = SpvOpFOrdLessThan;
                }

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, lessThan->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = lessThan->result;
                spv[3] = lessThan->lhs;
                spv[4] = lessThan->rhs;
                break;
            }
            case IL::OpCode::LessThanEqual: {
                auto *lessThanEqual = instr.As<IL::LessThanEqualInstruction>();

                const Backend::IL::Type* lhsType = ilTypeMap.GetType(lessThanEqual->lhs);

                SpvOp op;
                if (lhsType->kind == Backend::IL::TypeKind::Int) {
                    op = lhsType->As<Backend::IL::IntType>()->signedness ? SpvOpSLessThanEqual : SpvOpULessThanEqual;
                } else {
                    op = SpvOpFOrdLessThanEqual;
                }

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, lessThanEqual->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = lessThanEqual->result;
                spv[3] = lessThanEqual->lhs;
                spv[4] = lessThanEqual->rhs;
                break;
            }
            case IL::OpCode::GreaterThan: {
                auto *greaterThan = instr.As<IL::GreaterThanInstruction>();

                const Backend::IL::Type* lhsType = ilTypeMap.GetType(greaterThan->lhs);

                SpvOp op;
                if (lhsType->kind == Backend::IL::TypeKind::Int) {
                    op = lhsType->As<Backend::IL::IntType>()->signedness ? SpvOpSGreaterThan : SpvOpUGreaterThan;
                } else {
                    op = SpvOpFOrdGreaterThan;
                }

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, greaterThan->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = greaterThan->result;
                spv[3] = greaterThan->lhs;
                spv[4] = greaterThan->rhs;
                break;
            }
            case IL::OpCode::GreaterThanEqual: {
                auto *greaterThanEqual = instr.As<IL::GreaterThanEqualInstruction>();

                const Backend::IL::Type* lhsType = ilTypeMap.GetType(greaterThanEqual->lhs);

                SpvOp op;
                if (lhsType->kind == Backend::IL::TypeKind::Int) {
                    op = lhsType->As<Backend::IL::IntType>()->signedness ? SpvOpSGreaterThanEqual : SpvOpUGreaterThanEqual;
                } else {
                    op = SpvOpFOrdGreaterThanEqual;
                }

                SpvInstruction& spv = stream.TemplateOrAllocate(op, 5, greaterThanEqual->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = greaterThanEqual->result;
                spv[3] = greaterThanEqual->lhs;
                spv[4] = greaterThanEqual->rhs;
                break;
            }
            case IL::OpCode::Branch: {
                auto *branch = instr.As<IL::BranchInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpBranch, 2, branch->source);
                spv[1] = branch->branch;
                break;
            }
            case IL::OpCode::BranchConditional: {
                auto *branch = instr.As<IL::BranchConditionalInstruction>();

                // Get the blocks
                IL::BasicBlock* blockPass = fn.GetBlock(branch->pass);
                IL::BasicBlock* blockFail = fn.GetBlock(branch->fail);

                // Must be valid
                if (!blockPass || !blockFail) {
                    return false;
                }

                // Get the terminators
                auto terminatorPass = blockPass->GetTerminator().Get();
                auto terminatorFail = blockFail->GetTerminator().Get();

                // Resulting merge label, for structured cfg
                IL::ID cfgMergeLabel{InvalidSpvId};

                // Optional branch instructions
                auto passBranch = terminatorPass->Cast<IL::BranchInstruction>();
                auto failBranch = terminatorFail->Cast<IL::BranchInstruction>();

                // If the pass block branches back to the fail block, assume merge block
                if (passBranch) {
                    if (passBranch->branch == blockFail->GetID()) {
                        cfgMergeLabel = blockFail->GetID();
                    }
                }

                // If the fail block branches back to the success block, assume merge block
                if (failBranch) {
                    if (failBranch->branch == blockPass->GetID()) {
                        cfgMergeLabel = blockPass->GetID();
                    }
                }

                // If neither, check shared branches
                if (!cfgMergeLabel) {
                    // Must be non-conditional branches
                    if (!passBranch || !failBranch) {
                        return false;
                    }

                    // Not structured if diverging (not really true, but simplifies my life for now)
                    if (passBranch->branch != failBranch->branch) {
                        return false;
                    }

                    // Assume merge label
                    cfgMergeLabel = passBranch->branch;
                }

                // Write cfg
                SpvInstruction& cfg = stream.Allocate(SpvOpSelectionMerge, 3);
                cfg[1] = cfgMergeLabel;
                cfg[2] = SpvSelectionControlMaskNone;

                // Perform the branch, must be after cfg instruction
                SpvInstruction& spv = stream.Allocate(SpvOpBranchConditional, 4);
                spv[1] = branch->cond;
                spv[2] = branch->pass;
                spv[3] = branch->fail;
                break;
            }
            case IL::OpCode::BitOr: {
                auto *bitOr = instr.As<IL::BitOrInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpBitwiseOr, 5, bitOr->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = bitOr->result;
                spv[3] = bitOr->lhs;
                spv[4] = bitOr->rhs;
                break;
            }
            case IL::OpCode::BitAnd: {
                auto *bitAnd = instr.As<IL::BitAndInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpBitwiseAnd, 5, bitAnd->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = bitAnd->result;
                spv[3] = bitAnd->lhs;
                spv[4] = bitAnd->rhs;
                break;
            }
            case IL::OpCode::BitShiftLeft: {
                auto *bsl = instr.As<IL::BitShiftLeftInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpShiftLeftLogical, 5, bsl->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = bsl->result;
                spv[3] = bsl->value;
                spv[4] = bsl->shift;
                break;
            }
            case IL::OpCode::BitShiftRight: {
                auto *bsr = instr.As<IL::BitShiftRightInstruction>();

                SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpShiftRightLogical, 5, bsr->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = bsr->result;
                spv[3] = bsr->value;
                spv[4] = bsr->shift;
                break;
            }
            case IL::OpCode::Export: {
                auto *_export = instr.As<IL::ExportInstruction>();

                // Note: This is quite ugly, will be changed

                // UInt32
                Backend::IL::IntType typeInt;
                typeInt.bitWidth = 32;
                typeInt.signedness = false;
                const Backend::IL::Type* uintType = ilTypeMap.FindTypeOrAdd(typeInt);

                // Uint32*
                Backend::IL::PointerType typeUintImagePtr;
                typeUintImagePtr.pointee = uintType;
                typeUintImagePtr.addressSpace = Backend::IL::AddressSpace::Texture;
                const Backend::IL::Type* uintImagePtrType = ilTypeMap.FindTypeOrAdd(typeUintImagePtr);

                // Constant identifiers
                uint32_t zeroUintId = header.bound++;
                uint32_t streamOffsetId = header.bound++;
                uint32_t scopeId = header.bound++;
                uint32_t memSemanticId = header.bound++;
                uint32_t offsetAdditionId = header.bound++;

                // 0
                SpvInstruction& spv = declarationStream->Allocate(SpvOpConstant, 4);
                spv[1] = typeMap->GetSpvTypeId(uintType);
                spv[2] = zeroUintId;
                spv[3] = 0;

                // Index of the stream
                SpvInstruction& spvOffset = declarationStream->Allocate(SpvOpConstant, 4);
                spvOffset[1] = typeMap->GetSpvTypeId(uintType);
                spvOffset[2] = streamOffsetId;
                spvOffset[3] = _export->exportID;

                // Device scope
                SpvInstruction& spvScope = declarationStream->Allocate(SpvOpConstant, 4);
                spvScope[1] = typeMap->GetSpvTypeId(uintType);
                spvScope[2] = scopeId;
                spvScope[3] = SpvScopeDevice;

                // No memory mask
                SpvInstruction& spvMemSem = declarationStream->Allocate(SpvOpConstant, 4);
                spvMemSem[1] = typeMap->GetSpvTypeId(uintType);
                spvMemSem[2] = memSemanticId;
                spvMemSem[3] = SpvMemorySemanticsMaskNone;

                // The offset addition (will change for dynamic types in the future)
                SpvInstruction& spvSize = declarationStream->Allocate(SpvOpConstant, 4);
                spvSize[1] = typeMap->GetSpvTypeId(uintType);
                spvSize[2] = offsetAdditionId;
                spvSize[3] = 1u;

                uint32_t texelPtrId = header.bound++;

                // Get the address of the texel to be atomically incremented
                SpvInstruction& texelPtr = stream.Allocate(SpvOpImageTexelPointer, 6);
                texelPtr[1] = typeMap->GetSpvTypeId(uintImagePtrType);
                texelPtr[2] = texelPtrId;
                texelPtr[3] = metaData.exportMd.counterId;
                texelPtr[4] = streamOffsetId;
                texelPtr[5] = zeroUintId;

                uint32_t atomicPositionId = header.bound++;

                // Atomically increment the texel
                SpvInstruction& atom = stream.Allocate(SpvOpAtomicIAdd, 7);
                atom[1] = typeMap->GetSpvTypeId(uintType);
                atom[2] = atomicPositionId;
                atom[3] = texelPtrId;
                atom[4] = scopeId;
                atom[5] = memSemanticId;
                atom[6] = offsetAdditionId;

                uint32_t accessId = header.bound++;

                // Get the destination stream
                SpvInstruction& chain = stream.Allocate(SpvOpAccessChain, 5);
                chain[1] = typeMap->GetSpvTypeId(metaData.exportMd.buffer32UIRWPtr);
                chain[2] = accessId;
                chain[3] = metaData.exportMd.streamId;
                chain[4] = streamOffsetId;

                uint32_t accessLoadId = header.bound++;

                // Load the stream
                SpvInstruction &load = stream.Allocate(SpvOpLoad, 4);
                load[1] = typeMap->GetSpvTypeId(metaData.exportMd.buffer32UIRW);
                load[2] = accessLoadId;
                load[3] = accessId;

                // Write to the stream
                SpvInstruction &write = stream.Allocate(SpvOpImageWrite, 5);
                write[1] = accessLoadId;
                write[2] = atomicPositionId;
                write[3] = _export->value;
                write[4] = SpvImageOperandsMaskNone;

                break;
            }
            case IL::OpCode::Alloca: {
                auto *bsr = instr.As<IL::BitShiftRightInstruction>();

                SpvInstruction& spv = declarationStream->TemplateOrAllocate(SpvOpVariable, 4, bsr->source);
                spv[1] = typeMap->GetSpvTypeId(resultType);
                spv[2] = bsr->result;
                spv[3] = SpvStorageClassFunction;
                break;
            }
            case IL::OpCode::Load: {
                auto *load = instr.As<IL::LoadInstruction>();

                SpvInstruction& spv = declarationStream->TemplateOrAllocate(SpvOpLoad, 4, load->source);
                spv[1] = typeMap->GetSpvTypeId(resultType->As<Backend::IL::PointerType>()->pointee);
                spv[2] = load->result;
                spv[3] = SpvStorageClassFunction;
                break;
            }
            case IL::OpCode::Store: {
                auto *load = instr.As<IL::StoreInstruction>();

                SpvInstruction& spv = declarationStream->TemplateOrAllocate(SpvOpStore, 3, load->source);
                spv[1] = load->address;
                spv[2] = load->value;
                break;
            }
            case IL::OpCode::LoadBuffer: {
                auto *loadBuffer = instr.As<IL::LoadBufferInstruction>();

                // Get the buffer type
                const auto* bufferType = ilTypeMap.GetType(loadBuffer->buffer)->As<Backend::IL::BufferType>();

                // Texel buffer?
                if (bufferType->texelType != Backend::IL::Format::None) {
                    // Load image
                    SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpImageRead, 5, instr->source);
                    spv[1] = typeMap->GetSpvTypeId(bufferType->elementType);
                    spv[2] = loadBuffer->result;
                    spv[3] = loadBuffer->buffer;
                    spv[4] = loadBuffer->index;
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
                    spv[1] = storeBuffer->buffer;
                    spv[2] = storeBuffer->index;
                    spv[3] = storeBuffer->value;
                } else {
                    ASSERT(false, "Not implemented");
                    return false;
                }
                break;
            }
            case IL::OpCode::ResourceSize: {
                auto *size = instr.As<IL::ResourceSizeInstruction>();

                // Get the resource type
                const auto* resourceType = ilTypeMap.GetType(size->resource);

                switch (resourceType->kind) {
                    default:{
                        ASSERT(false, "Invalid ResourceSize type kind");
                        return false;
                    }
                    case Backend::IL::TypeKind::Texture: {
                        // Query image
                        SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpImageQuerySize, 4, instr->source);
                        spv[1] = typeMap->GetSpvTypeId(resultType);
                        spv[2] = size->result;
                        spv[3] = size->resource;
                        break;
                    }
                    case Backend::IL::TypeKind::Buffer: {
                        // Texel buffer?
                        if (resourceType->As<Backend::IL::BufferType>()->texelType != Backend::IL::Format::None) {
                            // Query image
                            SpvInstruction& spv = stream.TemplateOrAllocate(SpvOpImageQuerySize, 4, instr->source);
                            spv[1] = typeMap->GetSpvTypeId(resultType);
                            spv[2] = size->result;
                            spv[3] = size->resource;
                        } else {
                            ASSERT(false, "Not implemented");
                        }
                        break;
                    }
                }

                break;
            }
        }
    }

    return true;
}
