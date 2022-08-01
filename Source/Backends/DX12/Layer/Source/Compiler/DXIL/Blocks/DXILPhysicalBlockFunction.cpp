#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockFunction.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordReader.h>
#include <Backends/DX12/Compiler/DXIL/DXILIDMap.h>
#include <Backends/DX12/Compiler/DXIL/DXIL.Gen.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBitStreamReader.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 *
 * Loosely derived from LLVM BitcodeWriter
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/lib/Bitcode/Writer/BitcodeWriter.cpp
 */

void DXILPhysicalBlockFunction::ParseFunction(struct LLVMBlock *block) {
    // Definition order is linear to the internally linked functions
    uint32_t linkedIndex = internalLinkedFunctions[program.GetFunctionList().GetCount()];

    // Get function definition
    DXILFunctionDeclaration &declaration = functions[linkedIndex];

    // Get type map
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();

    // Create function
    IL::Function *fn = program.GetFunctionList().AllocFunction();

    // Set the type
    fn->SetFunctionType(declaration.type);

    // Visit child blocks
    for (LLVMBlock *fnBlock: block->blocks) {
        switch (static_cast<LLVMReservedBlock>(fnBlock->id)) {
            default: {
                break;
            }
            case LLVMReservedBlock::Constants: {
                table.global.ParseConstants(fnBlock);
                break;
            }
        }
    }

    // Create parameter mappings
    for (uint32_t i = 0; i < declaration.type->parameterTypes.size(); i++) {
        declaration.parameters.Add(table.idMap.GetAnchor());
        table.idMap.AllocMappedID(DXILIDType::Parameter);
    }

    // Allocate basic block
    IL::BasicBlock *basicBlock = fn->GetBasicBlocks().AllocBlock();

    // Local block mappings
    TrivialStackVector<IL::BasicBlock *, 32> blockMapping;
    blockMapping.Add(basicBlock);

    // Current block index
    uint32_t blockIndex{0};

    // Reserve forward allocations
    table.idMap.ReserveForward(block->records.Size());

    // Visit function records
    for (size_t recordIdx = 0; recordIdx < block->records.Size(); recordIdx++) {
        LLVMRecord &record = block->records[recordIdx];

        LLVMRecordReader reader(record);

        // Get the current id anchor
        //   LLVM id references are encoded relative to the current record
        uint32_t anchor = table.idMap.GetAnchor();

        // Optional record result
        IL::ID result = IL::InvalidID;

        // Create mapping if present
        if (HasResult(record)) {
            result = table.idMap.AllocMappedID(DXILIDType::Instruction);
        }

        // Handle instruction
        switch (static_cast<LLVMFunctionRecord>(record.id)) {
            default: {
                ASSERT(false, "Unexpected function record");
                return;
            }

            case LLVMFunctionRecord::InstInvoke:
            case LLVMFunctionRecord::InstUnwind:
            case LLVMFunctionRecord::InstFree:
            case LLVMFunctionRecord::InstVaArg:
            case LLVMFunctionRecord::InstIndirectBR:
            case LLVMFunctionRecord::InstGetResult:
            case LLVMFunctionRecord::InstMalloc: {
                ASSERT(false, "Unsupported instruction");
                return;
            }

            case LLVMFunctionRecord::DeclareBlocks: {
                const uint32_t blockCount = reader.ConsumeOp();

                // Allocate all blocks (except entry)
                for (uint32_t i = 0; i < blockCount - 1; i++) {
                    IL::ID id = program.GetIdentifierMap().AllocID();
                    blockMapping.Add(fn->GetBasicBlocks().AllocBlock(id));
                }
                break;
            }
            case LLVMFunctionRecord::InstBinOp: {
                uint64_t lhs = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                uint64_t rhs = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());

                // Create type mapping
                ilTypeMap.SetType(result, ilTypeMap.GetType(lhs));

                auto binOp = static_cast<LLVMBinOp>(reader.ConsumeOp());
                switch (binOp) {
                    default: {
                        ASSERT(false, "Unexpected binary operation");
                        return;
                    }

                    case LLVMBinOp::Add: {
                        IL::AddInstruction instr{};
                        instr.opCode = IL::OpCode::Add;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                    case LLVMBinOp::Sub: {
                        IL::SubInstruction instr{};
                        instr.opCode = IL::OpCode::Sub;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                    case LLVMBinOp::Mul: {
                        IL::MulInstruction instr{};
                        instr.opCode = IL::OpCode::Mul;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                    case LLVMBinOp::UDiv:
                    case LLVMBinOp::SDiv: {
                        IL::DivInstruction instr{};
                        instr.opCode = IL::OpCode::Div;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                    case LLVMBinOp::URem:
                    case LLVMBinOp::SRem: {
                        IL::RemInstruction instr{};
                        instr.opCode = IL::OpCode::Rem;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                    case LLVMBinOp::SHL: {
                        IL::BitShiftLeftInstruction instr{};
                        instr.opCode = IL::OpCode::BitShiftLeft;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.value = lhs;
                        instr.shift = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                    case LLVMBinOp::LShR:
                    case LLVMBinOp::AShR: {
                        IL::BitShiftRightInstruction instr{};
                        instr.opCode = IL::OpCode::BitShiftRight;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.value = lhs;
                        instr.shift = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                    case LLVMBinOp::And: {
                        IL::AndInstruction instr{};
                        instr.opCode = IL::OpCode::And;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                    case LLVMBinOp::Or: {
                        IL::OrInstruction instr{};
                        instr.opCode = IL::OpCode::Or;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                    case LLVMBinOp::XOr: {
                        IL::BitXOrInstruction instr{};
                        instr.opCode = IL::OpCode::BitXOr;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                }
                break;
            }
            case LLVMFunctionRecord::InstCast: {
                uint64_t value = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());

                // Create type mapping
                ilTypeMap.SetType(result, table.type.typeMap.GetType(reader.ConsumeOp()));

                auto castOp = static_cast<LLVMCastOp>(reader.ConsumeOp());
                switch (castOp) {
                    default: {
                        ASSERT(false, "Unexpected cast operation");
                        return;
                    }

                    case LLVMCastOp::Trunc:
                    case LLVMCastOp::FPTrunc: {
                        IL::TruncInstruction instr{};
                        instr.opCode = IL::OpCode::Trunc;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.value = value;
                        basicBlock->Append(instr);
                        break;
                    }

                    case LLVMCastOp::PtrToInt:
                    case LLVMCastOp::IntToPtr:
                    case LLVMCastOp::ZExt:
                    case LLVMCastOp::FPExt:
                    case LLVMCastOp::SExt: {
                        // Emit as unexposed
                        IL::UnexposedInstruction instr{};
                        instr.opCode = IL::OpCode::Unexposed;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.backendOpCode = record.id;
                        basicBlock->Append(instr);
                        break;
                    }

                    case LLVMCastOp::FPToUI:
                    case LLVMCastOp::FPToSI: {
                        IL::FloatToIntInstruction instr{};
                        instr.opCode = IL::OpCode::FloatToInt;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.value = value;
                        basicBlock->Append(instr);
                        break;
                    }

                    case LLVMCastOp::UIToFP:
                    case LLVMCastOp::SIToFP: {
                        IL::IntToFloatInstruction instr{};
                        instr.opCode = IL::OpCode::IntToFloat;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.value = value;
                        basicBlock->Append(instr);
                        break;
                    }

                    case LLVMCastOp::BitCast: {
                        IL::BitCastInstruction instr{};
                        instr.opCode = IL::OpCode::BitCast;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.value = value;
                        basicBlock->Append(instr);
                        break;
                    }
                }
                break;
            }


                /* Structural */
            case LLVMFunctionRecord::InstGEP:
            case LLVMFunctionRecord::InstSelect:
            case LLVMFunctionRecord::InstExtractELT:
            case LLVMFunctionRecord::InstInsertELT:
            case LLVMFunctionRecord::InstExtractVal:
            case LLVMFunctionRecord::InstInsertVal:
            case LLVMFunctionRecord::InstVSelect:
            case LLVMFunctionRecord::InstInBoundsGEP: {
                // Create type mapping
                ilTypeMap.SetType(result, ilTypeMap.FindTypeOrAdd(Backend::IL::UnexposedType{}));

                // Emit as unexposed
                IL::UnexposedInstruction instr{};
                instr.opCode = IL::OpCode::Unexposed;
                instr.result = result;
                instr.source = IL::Source::Code(recordIdx);
                instr.backendOpCode = record.id;
                basicBlock->Append(instr);
                break;
            }

                /* Inbuilt vector */
            case LLVMFunctionRecord::InstShuffleVec: {
                // Create type mapping
                ilTypeMap.SetType(result, ilTypeMap.FindTypeOrAdd(Backend::IL::UnexposedType{}));

                // Emit as unexposed
                IL::UnexposedInstruction instr{};
                instr.opCode = IL::OpCode::Unexposed;
                instr.result = result;
                instr.source = IL::Source::Code(recordIdx);
                instr.backendOpCode = record.id;
                basicBlock->Append(instr);
                break;
            }

            case LLVMFunctionRecord::InstCmp:
            case LLVMFunctionRecord::InstCmp2: {
                uint64_t lhs = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                uint64_t rhs = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());

                // Create type mapping
                ilTypeMap.SetType(result, ilTypeMap.FindTypeOrAdd(Backend::IL::BoolType{}));

                auto cmpOp = static_cast<LLVMCmpOp>(reader.ConsumeOp());
                switch (cmpOp) {
                    default: {
                        ASSERT(false, "Unexpected comparison operation");
                        return;
                    }

                    case LLVMCmpOp::FloatFalse:
                    case LLVMCmpOp::FloatTrue:
                    case LLVMCmpOp::BadFloatPredecate:
                    case LLVMCmpOp::IntBadPredecate:
                    case LLVMCmpOp::FloatOrdered:
                    case LLVMCmpOp::FloatNotOrdered: {
                        // Emit as unexposed
                        IL::UnexposedInstruction instr{};
                        instr.opCode = IL::OpCode::Unexposed;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.backendOpCode = record.id;
                        basicBlock->Append(instr);
                        break;
                    }

                    case LLVMCmpOp::FloatUnorderedNotEqual:
                    case LLVMCmpOp::FloatOrderedNotEqual:
                    case LLVMCmpOp::IntNotEqual: {
                        // Emit as unexposed
                        IL::NotEqualInstruction instr{};
                        instr.opCode = IL::OpCode::NotEqual;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }

                    case LLVMCmpOp::FloatOrderedEqual:
                    case LLVMCmpOp::FloatUnorderedEqual:
                    case LLVMCmpOp::IntEqual: {
                        // Emit as unexposed
                        IL::EqualInstruction instr{};
                        instr.opCode = IL::OpCode::Equal;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }

                    case LLVMCmpOp::FloatOrderedGreaterThan:
                    case LLVMCmpOp::IntUnsignedGreaterThan:
                    case LLVMCmpOp::IntSignedGreaterThan:
                    case LLVMCmpOp::FloatUnorderedGreaterThan: {
                        // Emit as unexposed
                        IL::GreaterThanInstruction instr{};
                        instr.opCode = IL::OpCode::GreaterThan;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }

                    case LLVMCmpOp::FloatOrderedLessThan:
                    case LLVMCmpOp::IntUnsignedLessThan:
                    case LLVMCmpOp::IntSignedLessThan:
                    case LLVMCmpOp::FloatUnorderedLessThan: {
                        // Emit as unexposed
                        IL::LessThanInstruction instr{};
                        instr.opCode = IL::OpCode::LessThan;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }

                    case LLVMCmpOp::FloatOrderedGreaterEqual:
                    case LLVMCmpOp::IntUnsignedGreaterEqual:
                    case LLVMCmpOp::IntSignedGreaterEqual:
                    case LLVMCmpOp::FloatUnorderedGreaterEqual: {
                        // Emit as unexposed
                        IL::GreaterThanEqualInstruction instr{};
                        instr.opCode = IL::OpCode::GreaterThanEqual;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }

                    case LLVMCmpOp::FloatOrderedLessEqual:
                    case LLVMCmpOp::IntUnsignedLessEqual:
                    case LLVMCmpOp::IntSignedLessEqual:
                    case LLVMCmpOp::FloatUnorderedLessEqual: {
                        // Emit as unexposed
                        IL::LessThanEqualInstruction instr{};
                        instr.opCode = IL::OpCode::LessThanEqual;
                        instr.result = result;
                        instr.source = IL::Source::Code(recordIdx);
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                }
                break;
            }

            case LLVMFunctionRecord::InstRet: {
                // Emit as unexposed
                IL::ReturnInstruction instr{};
                instr.opCode = IL::OpCode::Return;
                instr.result = result;
                instr.source = IL::Source::Code(recordIdx);
                instr.value = reader.ConsumeOpDefault(IL::InvalidID);

                // Mapping
                if (instr.value != IL::InvalidID) {
                    instr.value = table.idMap.GetMappedRelative(anchor, instr.value);
                }

                basicBlock->Append(instr);

                // Advance block, otherwise assume last record
                if (++blockIndex < blockMapping.Size()) {
                    basicBlock = blockMapping[blockIndex];
                } else {
                    basicBlock = nullptr;
                }
                break;
            }
            case LLVMFunctionRecord::InstBr: {
                uint64_t pass = blockMapping[reader.ConsumeOp()]->GetID();

                // Conditional?
                if (reader.Any()) {
                    IL::BranchConditionalInstruction instr{};
                    instr.opCode = IL::OpCode::BranchConditional;
                    instr.result = result;
                    instr.source = IL::Source::Code(recordIdx);
                    instr.pass = pass;
                    instr.fail = blockMapping[reader.ConsumeOp()]->GetID();
                    instr.cond = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                    basicBlock->Append(instr);
                } else {
                    IL::BranchInstruction instr{};
                    instr.opCode = IL::OpCode::Branch;
                    instr.result = result;
                    instr.source = IL::Source::Code(recordIdx);
                    instr.branch = pass;
                    basicBlock->Append(instr);
                }

                // Advance block, otherwise assume last record
                if (++blockIndex < blockMapping.Size()) {
                    basicBlock = blockMapping[blockIndex];
                } else {
                    basicBlock = nullptr;
                }
                break;
            }
            case LLVMFunctionRecord::InstSwitch: {
                reader.ConsumeOp();

                uint64_t value = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                uint64_t _default = blockMapping[reader.ConsumeOp()]->GetID();

                // Get remaining count
                uint64_t remaining = reader.Remaining();
                ASSERT(remaining % 2 == 0, "Unexpected record switch operation count");

                // Determine number of cases
                const uint32_t caseCount = remaining / 2;

                // Create instruction
                auto *instr = ALLOCA_SIZE(IL::SwitchInstruction, IL::SwitchInstruction::GetSize(caseCount));
                instr->opCode = IL::OpCode::Switch;
                instr->result = IL::InvalidID;
                instr->source = IL::Source::Code(recordIdx);
                instr->value = value;
                instr->_default = _default;
                instr->controlFlow = {};
                instr->cases.count = caseCount;

                // Fill cases
                for (uint32_t i = 0; i < caseCount; i++) {
                    IL::SwitchCase _case;
                    _case.literal = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                    _case.branch = blockMapping[reader.ConsumeOp()]->GetID();
                    instr->cases[i] = _case;
                }

                basicBlock->Append(instr);

                // Advance block, otherwise assume last record
                if (++blockIndex < blockMapping.Size()) {
                    basicBlock = blockMapping[blockIndex];
                } else {
                    basicBlock = nullptr;
                }
                break;
            }

            case LLVMFunctionRecord::InstUnreachable: {
                // Emit as unexposed
                IL::UnexposedInstruction instr{};
                instr.opCode = IL::OpCode::Unexposed;
                instr.result = result;
                instr.source = IL::Source::Code(recordIdx);
                instr.backendOpCode = record.id;
                basicBlock->Append(instr);

                // Advance block, otherwise assume last record
                if (++blockIndex < blockMapping.Size()) {
                    basicBlock = blockMapping[blockIndex];
                } else {
                    basicBlock = nullptr;
                }
                break;
            }

            case LLVMFunctionRecord::InstPhi: {
                // Create type mapping
                ilTypeMap.SetType(result, table.type.typeMap.GetType(reader.ConsumeOp()));

                // Get remaining count
                uint64_t remaining = reader.Remaining();
                ASSERT(remaining % 2 == 0, "Unexpected record phi operation count");

                // Determine number of values
                const uint32_t valueCount = remaining / 2;

                // Create instruction
                auto *instr = ALLOCA_SIZE(IL::PhiInstruction, IL::PhiInstruction::GetSize(valueCount));
                instr->opCode = IL::OpCode::Phi;
                instr->result = result;
                instr->source = IL::Source::Code(recordIdx);
                instr->values.count = valueCount;

                // Fill cases
                for (uint32_t i = 0; i < valueCount; i++) {
                    IL::PhiValue value;

                    // Decode value
                    int64_t signedValue = LLVMBitStreamReader::DecodeSigned(reader.ConsumeOp());
                    if (signedValue >= 0) {
                        value.value = table.idMap.GetMappedRelative(anchor, static_cast<uint32_t>(signedValue));
                    } else {
                        value.value = table.idMap.GetMappedForward(anchor, static_cast<uint32_t>(-signedValue));
                    }

                    value.branch = blockMapping[reader.ConsumeOp()]->GetID();
                    instr->values[i] = value;
                }

                // Append dynamic
                basicBlock->Append(instr);
                break;
            }

            case LLVMFunctionRecord::InstAlloca: {
                const Backend::IL::Type *type = table.type.typeMap.GetType(reader.ConsumeOp());

                // Create type mapping
                ilTypeMap.SetType(result, type);

                uint64_t sizeType = reader.ConsumeOp();
                uint64_t size = reader.ConsumeOp();

                // Append
                IL::AllocaInstruction instr{};
                instr.opCode = IL::OpCode::Alloca;
                instr.result = result;
                instr.source = IL::Source::Code(recordIdx);
                instr.type = type->id;
                basicBlock->Append(instr);
                break;
            }

            case LLVMFunctionRecord::InstLoad: {
                uint32_t address = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());

                // Get address type
                const Backend::IL::Type *type = ilTypeMap.GetType(address);

                // Set as pointee type
                if (auto pointer = type->Cast<Backend::IL::PointerType>()) {
                    ilTypeMap.SetType(result, pointer->pointee);
                } else {
                    ilTypeMap.SetType(result, ilTypeMap.FindTypeOrAdd(Backend::IL::UnexposedType{}));
                }

                // Append
                IL::LoadInstruction instr{};
                instr.opCode = IL::OpCode::Load;
                instr.result = result;
                instr.source = IL::Source::Code(recordIdx);
                instr.address = address;
                basicBlock->Append(instr);
                break;
            }

            case LLVMFunctionRecord::InstStore: {
                // Append
                IL::StoreInstruction instr{};
                instr.opCode = IL::OpCode::Store;
                instr.result = result;
                instr.source = IL::Source::Code(recordIdx);
                instr.address = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());

                // Type
                reader.ConsumeOp();

                instr.value = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                basicBlock->Append(instr);
                break;
            }
            case LLVMFunctionRecord::InstStore2: {
                // Append
                IL::StoreInstruction instr{};
                instr.opCode = IL::OpCode::Store;
                instr.result = result;
                instr.source = IL::Source::Code(recordIdx);
                instr.address = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());

                // Type
                reader.ConsumeOp();

                instr.value = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                basicBlock->Append(instr);
                break;
            }

            case LLVMFunctionRecord::InstCall:
            case LLVMFunctionRecord::InstCall2: {
                // Get attributes
                uint64_t attributes = reader.ConsumeOp();

                // Get packed convention
                uint64_t callingConvAndTailCall = reader.ConsumeOp();

                // Parse calling conventions
                auto callConv = static_cast<LLVMCallingConvention>((callingConvAndTailCall >> 1) & 1023);
                bool isTailCall = callingConvAndTailCall & 0x1;
                bool isMustTailCall = (callingConvAndTailCall >> 14) & 0x1;

                // Get type of function
                uint64_t type = reader.ConsumeOp();

                // Get callee
                uint64_t called = table.idMap.GetRelative(anchor, reader.ConsumeOp());

                // Get call declaration
                const DXILFunctionDeclaration *callDecl = GetFunctionDeclaration(called);

                // Create mapping if present
                if (!callDecl->type->returnType->Is<Backend::IL::VoidType>()) {
                    result = table.idMap.AllocMappedID(DXILIDType::Instruction);

                    // Set as return type
                    ilTypeMap.SetType(result, callDecl->type->returnType);
                }

                // Try intrinsic
                if (!TryParseIntrinsic(basicBlock, recordIdx, reader, anchor, called, result, callDecl)) {
                    // Unknown, emit as unexposed
                    IL::UnexposedInstruction instr{};
                    instr.opCode = IL::OpCode::Unexposed;
                    instr.result = result;
                    instr.source = IL::Source::Code(recordIdx);
                    instr.backendOpCode = record.id;
                    instr.symbol = table.symbol.GetValueAllocation(called);
                    basicBlock->Append(instr);
                }
                break;
            }

                /* Debug */
            case LLVMFunctionRecord::DebugLOC:
            case LLVMFunctionRecord::DebugLOCAgain:
            case LLVMFunctionRecord::DebugLOC2: {
                // Nothing yet
                break;
            }
        }

        // Set result
        record.SetSource(result != IL::InvalidID, anchor);
    }

    // Validation
    ASSERT(blockIndex == blockMapping.Size(), "Terminator to block count mismatch");
}

bool DXILPhysicalBlockFunction::HasResult(const struct LLVMRecord &record) {
    switch (static_cast<LLVMFunctionRecord>(record.id)) {
        default: {
            ASSERT(false, "Unexpected LLVM function record");
            return false;
        }

            /* Unsupported functions */
        case LLVMFunctionRecord::InstInvoke:
        case LLVMFunctionRecord::InstUnwind:
        case LLVMFunctionRecord::InstFree:
        case LLVMFunctionRecord::InstVaArg:
        case LLVMFunctionRecord::InstIndirectBR:
        case LLVMFunctionRecord::InstMalloc: {
            ASSERT(false, "Unsupported instruction");
            return false;
        }

        case LLVMFunctionRecord::DeclareBlocks:
            return false;
        case LLVMFunctionRecord::InstBinOp:
            return true;
        case LLVMFunctionRecord::InstCast:
            return true;
        case LLVMFunctionRecord::InstGEP:
            return true;
        case LLVMFunctionRecord::InstSelect:
            return true;
        case LLVMFunctionRecord::InstExtractELT:
            return true;
        case LLVMFunctionRecord::InstInsertELT:
            return true;
        case LLVMFunctionRecord::InstShuffleVec:
            return true;
        case LLVMFunctionRecord::InstCmp:
            return true;
        case LLVMFunctionRecord::InstRet:
            return record.opCount > 0;
        case LLVMFunctionRecord::InstBr:
            return false;
        case LLVMFunctionRecord::InstSwitch:
            return false;
        case LLVMFunctionRecord::InstUnreachable:
            return false;
        case LLVMFunctionRecord::InstPhi:
            return true;
        case LLVMFunctionRecord::InstAlloca:
            return true;
        case LLVMFunctionRecord::InstLoad:
            return true;
        case LLVMFunctionRecord::InstStore:
        case LLVMFunctionRecord::InstStoreOld:
        case LLVMFunctionRecord::InstStore2:
            return false;
        case LLVMFunctionRecord::InstCall:
        case LLVMFunctionRecord::InstCall2:
            // Handle in call
            return false;
        case LLVMFunctionRecord::InstGetResult:
            return true;
        case LLVMFunctionRecord::InstExtractVal:
            return true;
        case LLVMFunctionRecord::InstInsertVal:
            return true;
        case LLVMFunctionRecord::InstCmp2:
            return true;
        case LLVMFunctionRecord::InstVSelect:
            return true;
        case LLVMFunctionRecord::InstInBoundsGEP:
            return true;
        case LLVMFunctionRecord::DebugLOC:
            return false;
        case LLVMFunctionRecord::DebugLOCAgain:
            return false;
        case LLVMFunctionRecord::DebugLOC2:
            return false;
    }
}

void DXILPhysicalBlockFunction::ParseModuleFunction(struct LLVMRecord &record) {
    LLVMRecordReader reader(record);

    /*
     * LLVM Specification
     *   [FUNCTION, type, callingconv, isproto,
     *    linkage, paramattr, alignment, section, visibility, gc, prologuedata,
     *     dllstorageclass, comdat, prefixdata, personalityfn, preemptionspecifier]
     */

    // Allocate id to current function offset
    record.SetSource(true, table.idMap.GetAnchor());

    IL::ID id = table.idMap.AllocMappedID(DXILIDType::Function, functions.Size());

    // Create function
    DXILFunctionDeclaration &function = functions.Add();

    // Set id
    function.id = id;

    // Hash name
    function.hash = std::hash<std::string_view>{}(function.name);

    // Get function type
    uint64_t type = reader.ConsumeOp();
    function.type = table.type.typeMap.GetType(type)->As<Backend::IL::FunctionType>();

    // Ignored
    uint64_t callingConv = reader.ConsumeOp();
    uint64_t proto = reader.ConsumeOp();

    // Get function linkage
    function.linkage = static_cast<LLVMLinkage>(reader.ConsumeOp());

    // Ignored
    uint64_t paramAttr = reader.ConsumeOp();

    // Add to internal linked functions if not external
    if (proto == 0) {
        internalLinkedFunctions.Add(functions.Size() - 1);
    }
}

const DXILFunctionDeclaration *DXILPhysicalBlockFunction::GetFunctionDeclaration(uint32_t id) {
    ASSERT(table.idMap.GetType(id) == DXILIDType::Function, "Invalid function id");
    return &functions[table.idMap.GetDataIndex(id)];
}

bool DXILPhysicalBlockFunction::TryParseIntrinsic(IL::BasicBlock *basicBlock, uint32_t recordIdx, LLVMRecordReader &reader, uint32_t anchor, uint32_t called, uint32_t result, const DXILFunctionDeclaration *declaration) {
    LLVMRecordStringView view = table.symbol.GetValueString(called);

    // Get op-code
    uint64_t opCode = program.GetConstants().GetConstant<IL::IntConstant>(table.idMap.GetMappedRelative(anchor, reader.ConsumeOp()))->value;

    // Check hash
    switch (view.GetHash()) {
        default: {
            // Not an intrinsic
            return false;
        }

        /*
         * DXIL Specification
         *   declare %dx.types.Handle @dx.op.createHandle(
         *       i32,                  ; opcode
         *       i8,                   ; resource class: SRV=0, UAV=1, CBV=2, Sampler=3
         *       i32,                  ; resource range ID (constant)
         *       i32,                  ; index into the range
         *       i1)                   ; non-uniform resource index: false or true
         */

        case CRC64("dx.op.createHandle"): {
            if (view != "dx.op.createHandle") {
                return false;
            }

            // Resource class
            auto _class = static_cast<DXILShaderResourceClass>(program.GetConstants().GetConstant<IL::IntConstant>(table.idMap.GetMappedRelative(anchor, reader.ConsumeOp()))->value);

            // Binding
            uint64_t id = program.GetConstants().GetConstant<IL::IntConstant>(table.idMap.GetMappedRelative(anchor, reader.ConsumeOp()))->value;
            uint64_t rangeIndex = program.GetConstants().GetConstant<IL::IntConstant>(table.idMap.GetMappedRelative(anchor, reader.ConsumeOp()))->value;

            // Divergent?
            auto isNonUniform = program.GetConstants().GetConstant<IL::BoolConstant>(table.idMap.GetMappedRelative(anchor, reader.ConsumeOp()))->value;

            // Get the actual handle type
            auto type = table.metadata.GetHandleType(id);

            // Set as pointee type
            program.GetTypeMap().SetType(result, type);

            // Keep the original record
            IL::UnexposedInstruction instr{};
            instr.opCode = IL::OpCode::Unexposed;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            basicBlock->Append(instr);
            return true;
        }

        /*
         * DXIL Specification
         *   overloads: SM5.1: f16|f32|i16|i32,  SM6.0: f16|f32|f64|i8|i16|i32|i64
         *   declare void @dx.op.storeOutput.f32(
         *       i32,                            ; opcode
         *       i32,                            ; output ID
         *       i32,                            ; row (relative to start row of output ID)
         *       i8,                             ; column (relative to start column of output ID), constant in [0,3]
         *       float)                          ; value to store
         */

        case CRC64("dx.op.storeOutput.f32"): {
            if (view != "dx.op.storeOutput.f32") {
                return false;
            }

            uint64_t outputID = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
            uint64_t row = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
            uint64_t column = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
            uint64_t value = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());

            // Emit
            IL::StoreOutputInstruction instr{};
            instr.opCode = IL::OpCode::StoreOutput;
            instr.result = IL::InvalidID;
            instr.source = IL::Source::Code(recordIdx);
            instr.index = outputID;
            instr.row = row;
            instr.column = column;
            instr.value = value;
            basicBlock->Append(instr);
            return true;
        }

        /*
         * DXIL Specification
         *   ; overloads: SM5.1: f32|i32,  SM6.0: f32|i32
         *   ; returns: status
         *   declare %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(
         *       i32,                  ; opcode
         *       %dx.types.Handle,     ; resource handle
         *       i32,                  ; coordinate c0
         */

        case CRC64("dx.op.bufferLoad.f32"):
        case CRC64("dx.op.bufferLoad.i32"): {
            if (!view.StartsWith("dx.op.bufferLoad.")) {
                return false;
            }

            // Get operands, ignore offset for now
            uint64_t resource = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
            uint64_t coordinate = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
            uint64_t offset = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());

            // Emit as load
            IL::LoadBufferInstruction instr{};
            instr.opCode = IL::OpCode::LoadBuffer;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.buffer = resource;
            instr.index = coordinate;
            basicBlock->Append(instr);
            return true;
        }

        /*
         * DXIL Specification
         *   ; overloads: SM5.1: f32|i32,  SM6.0: f32|i32
         *   declare void @dx.op.bufferStore.f32(
         *       i32,                  ; opcode
         *       %dx.types.Handle,     ; resource handle
         *       i32,                  ; coordinate c0
         *       i32,                  ; coordinate c1
         *       float,                ; value v0
         *       float,                ; value v1
         *       float,                ; value v2
         *       float,                ; value v3
         *       i8)                   ; write mask
         */

        case CRC64("dx.op.bufferStore.f32"):
        case CRC64("dx.op.bufferStore.i32"): {
            if (!view.StartsWith("dx.op.bufferStore.")) {
                return false;
            }

            // Get operands, ignore offset for now
            uint64_t resource = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
            uint64_t coordinate = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
            uint64_t offset = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
            uint64_t x = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
            uint64_t y = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
            uint64_t z = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
            uint64_t w = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
            uint64_t mask = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());

            // Must match, if it needs to deviate then do translation instead
            static_assert(static_cast<uint32_t>(IL::ComponentMask::X) == BIT(0), "Unexpected color mask");
            static_assert(static_cast<uint32_t>(IL::ComponentMask::Y) == BIT(1), "Unexpected color mask");
            static_assert(static_cast<uint32_t>(IL::ComponentMask::Z) == BIT(2), "Unexpected color mask");
            static_assert(static_cast<uint32_t>(IL::ComponentMask::W) == BIT(3), "Unexpected color mask");

            // Emit as store
            IL::StoreBufferInstruction instr{};
            instr.opCode = IL::OpCode::StoreBuffer;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.buffer = resource;
            instr.index = coordinate;
            instr.value = IL::SOVValue(x, y, z, w);
            instr.mask = IL::ComponentMaskSet(mask);
            basicBlock->Append(instr);
            return true;
        }
    }
}

template<typename F>
void DXILPhysicalBlockFunction::CompileSVOX(IL::ID lhs, IL::ID rhs, F &&functor) {
    DXILIDUserType lhsIdType = table.idRemapper.GetUserMappingType(lhs);
    DXILIDUserType rhsIdType = table.idRemapper.GetUserMappingType(rhs);

    // Singular operations are pass through
    if (lhsIdType == DXILIDUserType::Singular) {
        ASSERT(rhsIdType == DXILIDUserType::Singular, "Singular operations must match");
        return functor(lhs, rhs);
    }

    // Get types
    const Backend::IL::Type* lhsType = program.GetTypeMap().GetType(lhs);
    const Backend::IL::Type* rhsType = program.GetTypeMap().GetType(rhs);

    // Get count
    uint32_t componentCount = lhsType->As<Backend::IL::VectorType>()->dimension;

    for (uint32_t i = 0; i < componentCount; i++) {

    }
}

void DXILPhysicalBlockFunction::CompileFunction(struct LLVMBlock *block) {
    // Remap all blocks by dominance
    for (IL::Function* fn : program.GetFunctionList()) {
        if (!fn->ReorderByDominantBlocks(false)) {
            return;
        }
    }

    LLVMBlock *constantBlock{nullptr};

    // Visit child blocks
    for (LLVMBlock *fnBlock: block->blocks) {
        switch (static_cast<LLVMReservedBlock>(fnBlock->id)) {
            default: {
                break;
            }
            case LLVMReservedBlock::Constants: {
                constantBlock = fnBlock;
                table.global.CompileConstants(fnBlock);
                break;
            }
        }
    }

    // If no constant block, create one
    if (!constantBlock) {
        constantBlock = block->blocks.Add(new(allocators) LLVMBlock);
        constantBlock->id = static_cast<uint32_t>(LLVMReservedBlock::Constants);
    }

    // Get the program map
    Backend::IL::TypeMap &typeMap = program.GetTypeMap();

    // Get functions
    IL::FunctionList &ilFunctions = program.GetFunctionList();

    // Not supported for the time being
    ASSERT(ilFunctions.GetCount() == 1, "More than one function present in program");
    IL::Function *fn = *ilFunctions.begin();

    // Swap source data
    TrivialStackVector<LLVMRecord, 32> source;
    block->records.Swap(source);

    // Swap element data
    TrivialStackVector<LLVMBlockElement, 128> elements;
    block->elements.Swap(elements);

    // Reserve
    block->elements.Reserve(elements.Size());

    // Filter all records
    for (const LLVMBlockElement &element: elements) {
        if (!element.Is(LLVMBlockElementType::Record)) {
            block->elements.Add(element);
        }
    }

    // Linear block to il mapper
    std::map<IL::ID, uint32_t> branchMappings;

    // Create mappings
    for (const IL::BasicBlock *bb: fn->GetBasicBlocks()) {
        branchMappings[bb->GetID()] = branchMappings.size();
    }

    // Emit the number of blocks
    LLVMRecord declareBlocks(LLVMFunctionRecord::DeclareBlocks);
    declareBlocks.opCount = 1;
    declareBlocks.ops = table.recordAllocator.AllocateArray<uint64_t>(1);
    declareBlocks.ops[0] = fn->GetBasicBlocks().GetBlockCount();
    block->InsertRecord(block->elements.begin(), declareBlocks);

    // Compile all blocks
    for (const IL::BasicBlock *bb: fn->GetBasicBlocks()) {
        // Compile all instructions
        for (const IL::Instruction *instr: *bb) {
            LLVMRecord record;

            // If it's valid, copy record
            if (instr->source.IsValid()) {
                // Copy the source
                record = source[instr->source.codeOffset];

                // If trivial, just send it off
                //   ? Branch dependent records are resolved immediately for branch remapping
                if (instr->source.TriviallyCopyable() && !IsBranchDependent(static_cast<LLVMFunctionRecord>(record.id))) {
                    block->AddRecord(record);
                    continue;
                } else {
                    // Preserve source anchor
                    record.SetUser(instr->result != IL::InvalidID, record.sourceAnchor, instr->result);
                }
            } else {
                // Entirely new record, user generated
                record.SetUser(instr->result != IL::InvalidID, ~0u, instr->result);
            }

            switch (instr->opCode) {
                default:
                ASSERT(false, "Invalid instruction in basic block");
                    break;
                case IL::OpCode::Literal: {
                    auto* _instr = instr->As<IL::LiteralInstruction>();

                    // Create constant
                    const Backend::IL::Constant* constant{nullptr};
                    switch (_instr->type) {
                        default: {
                            ASSERT(false, "Invalid literal instruction");
                            break;
                        }
                        case IL::LiteralType::Int: {
                            constant = program.GetConstants().FindConstantOrAdd(
                                program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=_instr->bitWidth, .signedness=_instr->signedness}),
                                Backend::IL::IntConstant{.value = _instr->value.integral}
                            );
                            break;
                        }
                        case IL::LiteralType::FP: {
                            constant = program.GetConstants().FindConstantOrAdd(
                                program.GetTypeMap().FindTypeOrAdd(Backend::IL::FPType{.bitWidth=_instr->bitWidth}),
                                Backend::IL::FPConstant{.value = _instr->value.fp}
                            );
                            break;
                        }
                    }

                    // Ensure allocation
                    table.global.constantMap.GetConstant(constant);

                    // Set redirection for constant
                    table.idRemapper.SetUserRedirect(instr->result, DXILIDRemapper::EncodeUserOperand(constant->id));
                    break;
                }

                    /* Binary ops */
                case IL::OpCode::Add:
                case IL::OpCode::Sub:
                case IL::OpCode::Div:
                case IL::OpCode::Mul:
                case IL::OpCode::BitOr:
                case IL::OpCode::BitXOr:
                case IL::OpCode::BitAnd:
                case IL::OpCode::BitShiftLeft:
                case IL::OpCode::BitShiftRight:
                case IL::OpCode::Rem:
                case IL::OpCode::Or:
                case IL::OpCode::And: {
                    // Prepare record
                    record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstBinOp);
                    record.opCount = 3;
                    record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);

                    // Translate op code
                    LLVMBinOp opCode{};
                    switch (instr->opCode) {
                        default:
                        ASSERT(false, "Unexpected opcode in instruction");
                            break;
                        case IL::OpCode::Add: {
                            auto _instr = instr->As<IL::AddInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->lhs);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->rhs);
                            opCode = LLVMBinOp::Add;
                            break;
                        }
                        case IL::OpCode::Sub: {
                            auto _instr = instr->As<IL::SubInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->lhs);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->rhs);
                            opCode = LLVMBinOp::Sub;
                            break;
                        }
                        case IL::OpCode::Div: {
                            auto _instr = instr->As<IL::DivInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->lhs);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->rhs);

                            const Backend::IL::Type *type = typeMap.GetType(_instr->lhs);
                            if (type->Is<Backend::IL::FPType>()) {
                                opCode = LLVMBinOp::SDiv;
                            } else if (auto intType = type->Cast<Backend::IL::IntType>()) {
                                opCode = intType->signedness ? LLVMBinOp::SDiv : LLVMBinOp::UDiv;
                            } else {
                                ASSERT(false, "Invalid type in Div");
                            }
                            break;
                        }
                        case IL::OpCode::Mul: {
                            auto _instr = instr->As<IL::MulInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->lhs);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->rhs);
                            opCode = LLVMBinOp::Mul;
                            break;
                        }
                        case IL::OpCode::Or: {
                            auto _instr = instr->As<IL::OrInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->lhs);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->rhs);
                            opCode = LLVMBinOp::Or;
                            break;
                        }
                        case IL::OpCode::BitOr: {
                            auto _instr = instr->As<IL::BitOrInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->lhs);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->rhs);
                            opCode = LLVMBinOp::Or;
                            break;
                        }
                        case IL::OpCode::BitXOr: {
                            auto _instr = instr->As<IL::BitXOrInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->lhs);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->rhs);
                            opCode = LLVMBinOp::XOr;
                            break;
                        }
                        case IL::OpCode::And: {
                            auto _instr = instr->As<IL::AndInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->lhs);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->rhs);
                            opCode = LLVMBinOp::And;
                            break;
                        }
                        case IL::OpCode::BitAnd: {
                            auto _instr = instr->As<IL::BitAndInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->lhs);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->rhs);
                            opCode = LLVMBinOp::And;
                            break;
                        }
                        case IL::OpCode::BitShiftLeft: {
                            auto _instr = instr->As<IL::BitShiftLeftInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->value);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->shift);
                            opCode = LLVMBinOp::SHL;
                            break;
                        }
                        case IL::OpCode::BitShiftRight: {
                            auto _instr = instr->As<IL::BitShiftRightInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->value);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->shift);
                            opCode = LLVMBinOp::AShR;
                            break;
                        }
                        case IL::OpCode::Rem: {
                            auto _instr = instr->As<IL::RemInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->lhs);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->rhs);

                            const Backend::IL::Type *type = typeMap.GetType(_instr->lhs);
                            if (type->Is<Backend::IL::FPType>()) {
                                opCode = LLVMBinOp::SRem;
                            } else if (auto intType = type->Cast<Backend::IL::IntType>()) {
                                opCode = intType->signedness ? LLVMBinOp::SRem : LLVMBinOp::URem;
                            } else {
                                ASSERT(false, "Invalid type in Rem");
                            }
                            break;
                        }
                    }

                    // Set bin op
                    record.ops[2] = static_cast<uint64_t>(opCode);
                    block->AddRecord(record);
                    break;
                }

                case IL::OpCode::ResourceSize: {
                    auto _instr = instr->As<IL::ResourceSizeInstruction>();

                    // Get intrinsic
                    const DXILFunctionDeclaration *intrinsic = GetResourceSizeIntrinsic();

                    /*
                     * declare %dx.types.Dimensions @dx.op.getDimensions(
                     *   i32,                  ; opcode
                     *   %dx.types.Handle,     ; resource handle
                     *   i32)                  ; MIP level
                     */

                    // TODO: Clean up, ugly

                    uint64_t ops[3];

                    ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                        Backend::IL::IntConstant{.value = static_cast<uint32_t>(DXILOpcodes::GetDimensions)}
                    )->id);

                    ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->resource);

                    // Buffer types are assigned undefined constants
                    if (typeMap.GetType(_instr->resource)->Is<Backend::IL::BufferType>()) {
                        ops[2] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                            Backend::IL::UndefConstant{}
                        )->id);
                    } else {
                        ops[2] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                            Backend::IL::IntConstant{.value = 0}
                        )->id);
                    }

                    // Scalar return?
                    if (!typeMap.GetType(_instr->result)->Is<Backend::IL::VectorType>()){
                        IL::ID structDimensions = program.GetIdentifierMap().AllocID();

                        // Invoke
                        block->AddRecord(CompileIntrinsicCall(structDimensions, intrinsic, 3, ops));

                        // Extract first value
                        LLVMRecord recordExtract(LLVMFunctionRecord::InstExtractVal);
                        recordExtract.SetUser(true, ~0u, _instr->result);
                        recordExtract.opCount = 2;
                        recordExtract.ops = table.recordAllocator.AllocateArray<uint64_t>(2);
                        recordExtract.ops[0] = DXILIDRemapper::EncodeUserOperand(structDimensions);
                        recordExtract.ops[1] = 0;
                        block->AddRecord(recordExtract);
                    } else {
                        // Invoke
                        block->AddRecord(CompileIntrinsicCall(_instr->result, intrinsic, 3, ops));
                    }
                    break;
                }

                case IL::OpCode::Equal:
                case IL::OpCode::NotEqual:
                case IL::OpCode::LessThan:
                case IL::OpCode::LessThanEqual:
                case IL::OpCode::GreaterThan:
                case IL::OpCode::GreaterThanEqual: {
                    // Prepare record
                    record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstCmp);
                    record.opCount = 3;
                    record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);

                    // Translate op code
                    LLVMCmpOp opCode{};
                    switch (instr->opCode) {
                        default:
                        ASSERT(false, "Unexpected opcode in instruction");
                            break;
                        case IL::OpCode::Equal: {
                            auto _instr = instr->As<IL::EqualInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->lhs);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->rhs);

                            const Backend::IL::Type *type = typeMap.GetType(_instr->lhs);
                            if (type->Is<Backend::IL::FPType>()) {
                                opCode = LLVMCmpOp::FloatUnorderedEqual;
                            } else {
                                opCode = LLVMCmpOp::IntEqual;
                            }
                            break;
                        }
                        case IL::OpCode::NotEqual: {
                            auto _instr = instr->As<IL::NotEqualInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->lhs);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->rhs);

                            const Backend::IL::Type *type = typeMap.GetType(_instr->lhs);
                            if (type->Is<Backend::IL::FPType>()) {
                                opCode = LLVMCmpOp::FloatUnorderedNotEqual;
                            } else {
                                opCode = LLVMCmpOp::IntNotEqual;
                            }
                            break;
                        }
                        case IL::OpCode::LessThan: {
                            auto _instr = instr->As<IL::LessThanInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->lhs);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->rhs);

                            const Backend::IL::Type *type = typeMap.GetType(_instr->lhs);
                            if (type->Is<Backend::IL::FPType>()) {
                                opCode = LLVMCmpOp::FloatUnorderedLessThan;
                            } else if (auto intType = type->Cast<Backend::IL::IntType>()) {
                                opCode = intType->signedness ? LLVMCmpOp::IntSignedLessThan : LLVMCmpOp::IntUnsignedLessThan;
                            } else {
                                ASSERT(false, "Invalid type in LessThan");
                            }
                            break;
                        }
                        case IL::OpCode::LessThanEqual: {
                            auto _instr = instr->As<IL::LessThanEqualInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->lhs);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->rhs);

                            const Backend::IL::Type *type = typeMap.GetType(_instr->lhs);
                            if (type->Is<Backend::IL::FPType>()) {
                                opCode = LLVMCmpOp::FloatUnorderedLessEqual;
                            } else if (auto intType = type->Cast<Backend::IL::IntType>()) {
                                opCode = intType->signedness ? LLVMCmpOp::IntSignedLessEqual : LLVMCmpOp::IntUnsignedLessEqual;
                            } else {
                                ASSERT(false, "Invalid type in LessThanEqual");
                            }
                            break;
                        }
                        case IL::OpCode::GreaterThan: {
                            auto _instr = instr->As<IL::GreaterThanInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->lhs);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->rhs);

                            const Backend::IL::Type *type = typeMap.GetType(_instr->lhs);
                            if (type->Is<Backend::IL::FPType>()) {
                                opCode = LLVMCmpOp::FloatUnorderedGreaterThan;
                            } else if (auto intType = type->Cast<Backend::IL::IntType>()) {
                                opCode = intType->signedness ? LLVMCmpOp::IntSignedGreaterThan : LLVMCmpOp::IntUnsignedGreaterThan;
                            } else {
                                ASSERT(false, "Invalid type in GreaterThan");
                            }
                            break;
                        }
                        case IL::OpCode::GreaterThanEqual: {
                            auto _instr = instr->As<IL::GreaterThanEqualInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->lhs);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->rhs);

                            const Backend::IL::Type *type = typeMap.GetType(_instr->lhs);
                            if (type->Is<Backend::IL::FPType>()) {
                                opCode = LLVMCmpOp::FloatUnorderedGreaterEqual;
                            } else if (auto intType = type->Cast<Backend::IL::IntType>()) {
                                opCode = intType->signedness ? LLVMCmpOp::IntSignedGreaterEqual : LLVMCmpOp::IntUnsignedGreaterEqual;
                            } else {
                                ASSERT(false, "Invalid type in GreaterThanEqual");
                            }
                            break;
                        }
                    }

                    // Set cmp op
                    record.ops[2] = static_cast<uint64_t>(opCode);
                    block->AddRecord(record);
                    break;
                }

                case IL::OpCode::Branch: {
                    auto _instr = instr->As<IL::BranchInstruction>();

                    // Prepare record
                    record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstBr);
                    record.opCount = 1;
                    record.ops = table.recordAllocator.AllocateArray<uint64_t>(1);
                    record.ops[0] = branchMappings.at(_instr->branch);
                    block->AddRecord(record);
                    break;
                }
                case IL::OpCode::BranchConditional: {
                    auto _instr = instr->As<IL::BranchConditionalInstruction>();

                    // Prepare record
                    record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstBr);
                    record.opCount = 3;
                    record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                    record.ops[0] = branchMappings.at(_instr->pass);
                    record.ops[1] = branchMappings.at(_instr->fail);
                    record.ops[2] = table.idRemapper.EncodeRedirectedUserOperand(_instr->cond);
                    block->AddRecord(record);
                    break;
                }
                case IL::OpCode::Switch: {
                    auto _instr = instr->As<IL::SwitchInstruction>();

                    // TODO: New switch statements
                    uint64_t type = record.ops ? record.ops[0] : 0;

                    // Prepare record
                    record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstSwitch);
                    record.opCount = 3 + 2 * _instr->cases.count;
                    record.ops = table.recordAllocator.AllocateArray<uint64_t>(record.opCount);
                    record.ops[0] = type;
                    record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->value);
                    record.ops[2] = branchMappings.at(_instr->_default);

                    for (uint32_t i = 0; i < _instr->cases.count; i++) {
                        record.ops[3 + i * 2] = table.idRemapper.EncodeRedirectedUserOperand(_instr->cases[i].literal);
                        record.ops[4 + i * 2] = branchMappings.at(_instr->cases[i].branch);
                    }
                    block->AddRecord(record);
                    break;
                }
                case IL::OpCode::Phi: {
                    auto _instr = instr->As<IL::PhiInstruction>();

                    // Prepare record
                    record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstPhi);
                    record.opCount = 1 + 2 * _instr->values.count;
                    record.ops = table.recordAllocator.AllocateArray<uint64_t>(record.opCount);
                    record.ops[0] = table.type.typeMap.GetType(program.GetTypeMap().GetType(_instr->result));

                    for (uint32_t i = 0; i < _instr->values.count; i++) {
                        record.ops[1 + i * 2] = table.idRemapper.EncodeRedirectedUserOperand(_instr->values[i].value);
                        record.ops[2 + i * 2] = branchMappings.at(_instr->values[i].branch);
                    }
                    block->AddRecord(record);
                    break;
                }
                case IL::OpCode::Return: {
                    auto _instr = instr->As<IL::ReturnInstruction>();

                    // Prepare record
                    record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstRet);

                    if (_instr->value != IL::InvalidID) {
                        record.opCount = 1;
                        record.ops[0] = _instr->value;
                    }
                    block->AddRecord(record);
                    break;
                }
                case IL::OpCode::Trunc:
                case IL::OpCode::FloatToInt:
                case IL::OpCode::IntToFloat:
                case IL::OpCode::BitCast: {
                    // Prepare record
                    record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstCast);
                    record.opCount = 3;
                    record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);

                    // Translate op code
                    LLVMCastOp opCode{};
                    switch (instr->opCode) {
                        default:
                        ASSERT(false, "Unexpected opcode in instruction");
                            break;
                        case IL::OpCode::Trunc: {
                            auto _instr = instr->As<IL::TruncInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->value);
                            opCode = LLVMCastOp::Trunc;
                            break;
                        }
                        case IL::OpCode::FloatToInt: {
                            auto _instr = instr->As<IL::FloatToIntInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->value);
                            opCode = LLVMCastOp::FPToUI;
                            break;
                        }
                        case IL::OpCode::IntToFloat: {
                            auto _instr = instr->As<IL::IntToFloatInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->value);
                            opCode = LLVMCastOp::SIToFP;
                            break;
                        }
                        case IL::OpCode::BitCast: {
                            auto _instr = instr->As<IL::BitCastInstruction>();
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->value);
                            opCode = LLVMCastOp::BitCast;
                            break;
                        }
                    }

                    // Assign type
                    record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(table.type.typeMap.GetType(program.GetTypeMap().GetType(record.ops[0])));

                    // Set cmp op
                    record.ops[2] = static_cast<uint64_t>(opCode);
                    block->AddRecord(record);
                    break;
                }

                case IL::OpCode::Any: {
                    // Dummy
                    table.idRemapper.SetUserRedirect(instr->result, table.global.constantMap.GetConstant(program.GetConstants().FindConstantOrAdd(
                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::BoolType{}),
                        Backend::IL::BoolConstant{.value = true}
                    )));

                    break;
                }

                case IL::OpCode::All: {
                    // Dummy
                    table.idRemapper.SetUserRedirect(instr->result, table.global.constantMap.GetConstant(program.GetConstants().FindConstantOrAdd(
                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::BoolType{}),
                        Backend::IL::BoolConstant{.value = true}
                    )));

                    break;
                }

                case IL::OpCode::Export: {
                    // NOOP
                    break;
                }

                    // To be implemented
#if 0
                    case IL::OpCode::Alloca:
                        break;
                    case IL::OpCode::Load:
                        break;
                    case IL::OpCode::Store:
                        break;
                    case IL::OpCode::StoreTexture:
                        break;
                    case IL::OpCode::LoadTexture:
                        break;
                    case IL::OpCode::StoreOutput:
                        break;
                    case IL::OpCode::StoreBuffer:
                        break;
                    case IL::OpCode::LoadBuffer:
                        break;
#endif
            }
        }
    }
}

void DXILPhysicalBlockFunction::CompileModuleFunction(LLVMRecord &record) {

}

void DXILPhysicalBlockFunction::StitchModuleFunction(LLVMRecord &record) {
    table.idRemapper.AllocRecordMapping(record);
}

void DXILPhysicalBlockFunction::StitchFunction(struct LLVMBlock *block) {
    // Definition order is linear to the internally linked functions
    const DXILFunctionDeclaration &declaration = functions[internalLinkedFunctions[stitchFunctionIndex++]];

    // Visit child blocks
    for (LLVMBlock *fnBlock: block->blocks) {
        switch (static_cast<LLVMReservedBlock>(fnBlock->id)) {
            default: {
                break;
            }
            case LLVMReservedBlock::Constants: {
                table.global.StitchConstants(fnBlock);
                break;
            }
        }
    }

    // Create parameter mappings
    for (uint32_t i = 0; i < declaration.parameters.Size(); i++) {
        table.idRemapper.AllocSourceMapping(declaration.parameters[i]);
    }

    // Visit function records
    //   ? +1, Skip DeclareBlocks
    for (size_t recordIdx = 1; recordIdx < block->records.Size(); recordIdx++) {
        LLVMRecord &record = block->records[recordIdx];

        // Current remapping anchor
        DXILIDRemapper::Anchor anchor = table.idRemapper.GetAnchor();

        // Allocate result
        if (record.hasValue) {
            table.idRemapper.AllocRecordMapping(record);
        }

        // Handle instruction
        switch (static_cast<LLVMFunctionRecord>(record.id)) {
            default: {
                // Force remap all operands as references
                for (uint32_t i = 0; i < record.opCount; i++) {
                    table.idRemapper.RemapRelative(anchor, record, record.ops[i]);
                }

                // OK
                break;
            }

            case LLVMFunctionRecord::InstExtractVal: {
                table.idRemapper.RemapRelative(anchor, record, record.Op(0));
                break;
            }

            case LLVMFunctionRecord::InstBinOp: {
                table.idRemapper.RemapRelative(anchor, record, record.Op(0));
                table.idRemapper.RemapRelative(anchor, record, record.Op(1));
                break;
            }

            case LLVMFunctionRecord::InstCast: {
                table.idRemapper.RemapRelative(anchor, record, record.Op(0));
                break;
            }

            case LLVMFunctionRecord::InstCmp:
            case LLVMFunctionRecord::InstCmp2: {
                table.idRemapper.RemapRelative(anchor, record, record.Op(0));
                table.idRemapper.RemapRelative(anchor, record, record.Op(1));
                break;
            }

            case LLVMFunctionRecord::InstRet: {
                if (record.opCount) {
                    table.idRemapper.RemapRelative(anchor, record, record.Op(0));
                }
                break;
            }
            case LLVMFunctionRecord::InstBr: {
                if (record.opCount > 1) {
                    table.idRemapper.RemapRelative(anchor, record, record.Op(2));
                }
                break;
            }
            case LLVMFunctionRecord::InstSwitch: {
                table.idRemapper.RemapRelative(anchor, record, record.Op(1));

                for (uint32_t i = 3; i < record.opCount; i += 2) {
                    table.idRemapper.RemapRelative(anchor, record, record.ops[i]);
                }
                break;
            }

            case LLVMFunctionRecord::InstPhi: {
                for (uint32_t i = 1; i < record.opCount; i += 2) {
                    table.idRemapper.RemapUnresolvedReference(anchor, record, record.ops[i]);
                }
                break;
            }

            case LLVMFunctionRecord::InstAlloca: {
                break;
            }

            case LLVMFunctionRecord::InstLoad: {
                table.idRemapper.RemapRelative(anchor, record, record.Op(0));
                break;
            }

            case LLVMFunctionRecord::InstStore:
            case LLVMFunctionRecord::InstStore2: {
                table.idRemapper.RemapRelative(anchor, record, record.Op(0));
                table.idRemapper.RemapRelative(anchor, record, record.Op(2));
                break;
            }

            case LLVMFunctionRecord::InstCall:
            case LLVMFunctionRecord::InstCall2: {
                table.idRemapper.RemapRelative(anchor, record, record.Op(3));

                for (uint32_t i = 4; i < record.opCount; i++) {
                    table.idRemapper.RemapRelative(anchor, record, record.ops[i]);
                }
                break;
            }
        }
    }
}

const DXILFunctionDeclaration *DXILPhysicalBlockFunction::GetResourceSizeIntrinsic() {
    if (intrinsics.resourceSize != IL::InvalidID) {
        return &functions[intrinsics.resourceSize];
    }

    /*
     * declare %dx.types.Dimensions @dx.op.getDimensions(
     *   i32,                  ; opcode
     *   %dx.types.Handle,     ; resource handle
     *   i32)                  ; MIP level
     */

    // RST symbol name
    const char *symbolName = "dx.op.getDimensions";

    // Search existing declarations
    for (const DXILFunctionDeclaration &decl: functions) {
        if (table.symbol.GetValueString(decl.id) == symbolName) {
            return &decl;
        }
    }

    // Root block
    LLVMBlock *global = &table.scan.GetRoot();

    // Get type map
    Backend::IL::TypeMap &typeMap = program.GetTypeMap();

    // Integral types
    const Backend::IL::Type *i8 = typeMap.FindTypeOrAdd(Backend::IL::IntType{.bitWidth = 8, .signedness = true});
    const Backend::IL::Type *i32 = typeMap.FindTypeOrAdd(Backend::IL::IntType{.bitWidth = 32, .signedness = true});

    // Opaque handle type
    Backend::IL::StructType handle;
    handle.memberTypes.push_back(typeMap.FindTypeOrAdd(Backend::IL::PointerType{
        .pointee = i8,
        .addressSpace = Backend::IL::AddressSpace::Function
    }));

    // Standard return
    Backend::IL::StructType ret4;
    ret4.memberTypes.push_back(i32);
    ret4.memberTypes.push_back(i32);
    ret4.memberTypes.push_back(i32);
    ret4.memberTypes.push_back(i32);

    // Export as special
    table.type.typeMap.CompileNamedType(typeMap.FindTypeOrAdd(ret4), "dx.types.Dimensions");

    // Function signature
    Backend::IL::FunctionType funcTy;
    funcTy.returnType = typeMap.FindTypeOrAdd(ret4);
    funcTy.parameterTypes.push_back(i32);
    funcTy.parameterTypes.push_back(typeMap.FindTypeOrAdd(handle));
    funcTy.parameterTypes.push_back(i32);
    const Backend::IL::FunctionType *fnType = typeMap.FindTypeOrAdd(funcTy);

    /*
     * LLVM Specification
     *   [FUNCTION, type, callingconv, isproto,
     *    linkage, paramattr, alignment, section, visibility, gc, prologuedata,
     *     dllstorageclass, comdat, prefixdata, personalityfn, preemptionspecifier]
     */

    // Insert after previous functions
    const LLVMBlockElement* insertionPoint = global->FindPlacementReverse(LLVMBlockElementType::Record, LLVMModuleRecord::Function) + 1;

    // Module scope function declaration
    LLVMRecord record(LLVMModuleRecord::Function);
    record.SetUser(true, ~0u, program.GetIdentifierMap().AllocID());
    record.opCount = 15;
    record.ops = table.recordAllocator.AllocateArray<uint64_t>(record.opCount);
    record.ops[0] = table.type.typeMap.GetType(fnType);
    record.ops[1] = static_cast<uint32_t>(LLVMCallingConvention::C);
    record.ops[2] = 1;
    record.ops[3] = static_cast<uint64_t>(LLVMLinkage::ExternalLinkage);

    LLVMParameterGroupValue attributes[] = { LLVMParameterGroupValue::NoUnwind, LLVMParameterGroupValue::ReadOnly };
    record.ops[4] = table.functionAttribute.FindOrCompileAttributeList(2, attributes);

    record.ops[5] = 0;
    record.ops[6] = 0;
    record.ops[7] = 0;
    record.ops[8] = 0;
    record.ops[9] = 0;
    record.ops[10] = 0;
    record.ops[11] = 0;
    record.ops[12] = 0;
    record.ops[13] = 0;
    record.ops[14] = 0;

    global->InsertRecord(insertionPoint, record);

    // Symbol tab for linking
    LLVMBlock *symTab = global->GetBlock(LLVMReservedBlock::ValueSymTab);

    // Symbol entry
    LLVMRecord syRecord(LLVMSymTabRecord::Entry);
    syRecord.SetUser(false, ~0u, ~0u);
    syRecord.opCount = 1 + std::strlen(symbolName);

    syRecord.ops = table.recordAllocator.AllocateArray<uint64_t>(syRecord.opCount);
    syRecord.ops[0] = DXILIDRemapper::EncodeUserOperand(record.result);

    // Copy name
    for (uint32_t i = 0; i < syRecord.opCount - 1; i++) {
        syRecord.ops[1 + i] = symbolName[i];
    }

    // Emit symbol
    symTab->AddRecord(syRecord);

    // Cache intrinsic
    intrinsics.resourceSize = functions.Size();

    // Create function
    DXILFunctionDeclaration &function = functions.Add();
    function.id = DXILIDRemapper::EncodeUserOperand(record.result);
    function.type = fnType;
    function.linkage = LLVMLinkage::CommonLinkage;
    return &function;
}

LLVMRecord DXILPhysicalBlockFunction::CompileIntrinsicCall(IL::ID result, const DXILFunctionDeclaration *decl, uint32_t opCount, const uint64_t *ops) {
    LLVMRecord record(LLVMFunctionRecord::InstCall2);
    record.SetUser(result != IL::InvalidID, ~0u, result);
    record.opCount = 4 + opCount;
    record.ops = table.recordAllocator.AllocateArray<uint64_t>(record.opCount);
    record.ops[0] = 0;
    record.ops[1] = 0;

    record.ops[1] = (0u << 1u);
    record.ops[1] |= static_cast<uint32_t>(LLVMCallingConvention::C) << 1;
    record.ops[1] |= (0u << 14u);
    record.ops[1] |= (1u << 15u);

    record.ops[2] = table.type.typeMap.GetType(decl->type);
    record.ops[3] = decl->id;

    // Emit call operands
    for (uint32_t i = 0; i < opCount; i++) {
        record.ops[4 + i] = ops[i];
    }

    // OK
    return record;
}

void DXILPhysicalBlockFunction::CopyTo(DXILPhysicalBlockFunction &out) {
    out.functions = functions;
    out.internalLinkedFunctions = internalLinkedFunctions;
}
