#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockFunction.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordReader.h>
#include <Backends/DX12/Compiler/DXIL/DXILIDMap.h>
#include <Backends/DX12/Compiler/DXIL/DXIL.Gen.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBitStream.h>

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 *
 * Loosely derived from LLVM BitcodeWriter
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/lib/Bitcode/Writer/BitcodeWriter.cpp
 */

void DXILPhysicalBlockFunction::ParseFunction(const struct LLVMBlock *block) {
    // Definition order is linear to the internally linked functions
    uint32_t linkedIndex = internalLinkedFunctions[program.GetFunctionList().GetCount()];

    // Get function definition
    const DXILFunctionDeclaration& declaration = functions[linkedIndex];

    // Create function
    IL::Function *fn = program.GetFunctionList().AllocFunction();

    // Visit child blocks
    for (const LLVMBlock *fnBlock: block->blocks) {
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
        table.idMap.AllocMappedID(DXILIDType::Parameter);
    }

    // Allocate basic block
    IL::BasicBlock *basicBlock = fn->GetBasicBlocks().AllocBlock();

    // Reserve forward allocations
    table.idMap.ReserveForward(block->records.Size());

    // Visit function records
    for (const LLVMRecord &record: block->records) {
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
                break;
            }
            case LLVMFunctionRecord::InstBinOp: {
                uint64_t lhs = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                uint64_t rhs = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());

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
                        instr.source = IL::Source::Invalid();
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                    case LLVMBinOp::Sub: {
                        IL::SubInstruction instr{};
                        instr.opCode = IL::OpCode::Sub;
                        instr.result = result;
                        instr.source = IL::Source::Invalid();
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                    case LLVMBinOp::Mul: {
                        IL::MulInstruction instr{};
                        instr.opCode = IL::OpCode::Mul;
                        instr.result = result;
                        instr.source = IL::Source::Invalid();
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
                        instr.source = IL::Source::Invalid();
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
                        instr.source = IL::Source::Invalid();
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                    case LLVMBinOp::SHL: {
                        IL::BitShiftLeftInstruction instr{};
                        instr.opCode = IL::OpCode::BitShiftLeft;
                        instr.result = result;
                        instr.source = IL::Source::Invalid();
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
                        instr.source = IL::Source::Invalid();
                        instr.value = lhs;
                        instr.shift = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                    case LLVMBinOp::And: {
                        IL::AndInstruction instr{};
                        instr.opCode = IL::OpCode::And;
                        instr.result = result;
                        instr.source = IL::Source::Invalid();
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                    case LLVMBinOp::Or: {
                        IL::OrInstruction instr{};
                        instr.opCode = IL::OpCode::Or;
                        instr.result = result;
                        instr.source = IL::Source::Invalid();
                        instr.lhs = lhs;
                        instr.rhs = rhs;
                        basicBlock->Append(instr);
                        break;
                    }
                    case LLVMBinOp::XOr: {
                        IL::BitXOrInstruction instr{};
                        instr.opCode = IL::OpCode::BitXOr;
                        instr.result = result;
                        instr.source = IL::Source::Invalid();
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

                reader.ConsumeOp();

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
                        instr.source = IL::Source::Invalid();
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
                        instr.source = IL::Source::Invalid();
                        instr.backendOpCode = record.id;
                        basicBlock->Append(instr);
                        break;
                    }

                    case LLVMCastOp::FPToUI:
                    case LLVMCastOp::FPToSI: {
                        IL::FloatToIntInstruction instr{};
                        instr.opCode = IL::OpCode::FloatToInt;
                        instr.result = result;
                        instr.source = IL::Source::Invalid();
                        instr.value = value;
                        basicBlock->Append(instr);
                        break;
                    }

                    case LLVMCastOp::UIToFP:
                    case LLVMCastOp::SIToFP: {
                        IL::IntToFloatInstruction instr{};
                        instr.opCode = IL::OpCode::IntToFloat;
                        instr.result = result;
                        instr.source = IL::Source::Invalid();
                        instr.value = value;
                        basicBlock->Append(instr);
                        break;
                    }

                    case LLVMCastOp::BitCast: {
                        IL::BitCastInstruction instr{};
                        instr.opCode = IL::OpCode::BitCast;
                        instr.result = result;
                        instr.source = IL::Source::Invalid();
                        instr.value = value;
                        basicBlock->Append(instr);
                        break;
                    }
                }

                // Append
                IL::LoadInstruction instr{};
                instr.opCode = IL::OpCode::Load;
                instr.result = result;
                instr.source = IL::Source::Invalid();
                instr.address = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                basicBlock->Append(instr);
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
                // Emit as unexposed
                IL::UnexposedInstruction instr{};
                instr.opCode = IL::OpCode::Unexposed;
                instr.result = result;
                instr.source = IL::Source::Invalid();
                instr.backendOpCode = record.id;
                basicBlock->Append(instr);
                break;
            }

                /* Inbuilt vector */
            case LLVMFunctionRecord::InstShuffleVec: {
                // Emit as unexposed
                IL::UnexposedInstruction instr{};
                instr.opCode = IL::OpCode::Unexposed;
                instr.result = result;
                instr.source = IL::Source::Invalid();
                instr.backendOpCode = record.id;
                basicBlock->Append(instr);
                break;
            }

            case LLVMFunctionRecord::InstCmp:
            case LLVMFunctionRecord::InstCmp2: {
                uint64_t lhs = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                uint64_t rhs = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());

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
                        instr.source = IL::Source::Invalid();
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
                        instr.source = IL::Source::Invalid();
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
                        instr.source = IL::Source::Invalid();
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
                        instr.source = IL::Source::Invalid();
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
                        instr.source = IL::Source::Invalid();
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
                        instr.source = IL::Source::Invalid();
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
                        instr.source = IL::Source::Invalid();
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
                instr.source = IL::Source::Invalid();
                instr.value = reader.ConsumeOpDefault(IL::InvalidID);

                if (instr.value != IL::InvalidID) {
                    instr.value = table.idMap.GetMappedRelative(anchor, instr.value);
                }

                basicBlock->Append(instr);
                break;
            }
            case LLVMFunctionRecord::InstBr: {
                uint64_t pass = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());

                // Conditional?
                if (reader.Any()) {
                    IL::BranchConditionalInstruction instr{};
                    instr.opCode = IL::OpCode::BranchConditional;
                    instr.result = result;
                    instr.source = IL::Source::Invalid();
                    instr.pass = pass;
                    instr.fail = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                    instr.cond = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                    basicBlock->Append(instr);
                } else {
                    IL::BranchInstruction instr{};
                    instr.opCode = IL::OpCode::Branch;
                    instr.result = result;
                    instr.source = IL::Source::Invalid();
                    instr.branch = pass;
                    basicBlock->Append(instr);
                }
                break;
            }
            case LLVMFunctionRecord::InstSwitch: {
                reader.ConsumeOp();

                uint64_t value = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                uint64_t _default = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());

                // Get remaining count
                uint64_t remaining = reader.Remaining();
                ASSERT(remaining % 2 == 0, "Unexpected record switch operation count");

                // Determine number of cases
                const uint32_t caseCount = remaining / 2;

                // Create instruction
                auto *instr = ALLOCA_SIZE(IL::SwitchInstruction, IL::SwitchInstruction::GetSize(caseCount));
                instr->opCode = IL::OpCode::Switch;
                instr->result = IL::InvalidID;
                instr->source = IL::Source::Invalid();
                instr->value = value;
                instr->_default = _default;
                instr->controlFlow = {};
                instr->cases.count = caseCount;

                // Fill cases
                for (uint32_t i = 0; i < caseCount; i++) {
                    IL::SwitchCase _case;
                    _case.literal = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                    _case.branch = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                    instr->cases[i] = _case;
                }

                basicBlock->Append(instr);
                break;
            }

            case LLVMFunctionRecord::InstUnreachable: {
                // Emit as unexposed
                IL::UnexposedInstruction instr{};
                instr.opCode = IL::OpCode::Unexposed;
                instr.result = result;
                instr.source = IL::Source::Invalid();
                instr.backendOpCode = record.id;
                basicBlock->Append(instr);
                break;
            }

            case LLVMFunctionRecord::InstPhi: {
                reader.ConsumeOp();

                // Get remaining count
                uint64_t remaining = reader.Remaining();
                ASSERT(remaining % 2 == 0, "Unexpected record phi operation count");

                // Determine number of values
                const uint32_t valueCount = remaining / 2;

                // Create instruction
                auto *instr = ALLOCA_SIZE(IL::PhiInstruction, IL::PhiInstruction::GetSize(valueCount));
                instr->opCode = IL::OpCode::Phi;
                instr->result = result;
                instr->source = IL::Source::Invalid();
                instr->values.count = valueCount;

                // Fill cases
                for (uint32_t i = 0; i < valueCount; i++) {
                    IL::PhiValue value;

                    // Decode value
                    int64_t signedValue = LLVMBitStream::DecodeSigned(reader.ConsumeOp());
                    if (signedValue >= 0) {
                        value.value = table.idMap.GetMappedRelative(anchor, static_cast<uint32_t>(signedValue));
                    } else {
                        value.value = table.idMap.GetMappedForward(anchor, static_cast<uint32_t>(-signedValue));
                    }

                    value.branch = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                    instr->values[i] = value;
                }

                // Append dynamic
                basicBlock->Append(instr);
                break;
            }

            case LLVMFunctionRecord::InstAlloca: {
                uint64_t type = reader.ConsumeOp();
                uint64_t sizeType = reader.ConsumeOp();
                uint64_t size = reader.ConsumeOp();

                // Append
                IL::AllocaInstruction instr{};
                instr.opCode = IL::OpCode::Alloca;
                instr.result = result;
                instr.source = IL::Source::Invalid();
                instr.type = type;
                basicBlock->Append(instr);
                break;
            }

            case LLVMFunctionRecord::InstLoad: {
                // Append
                IL::LoadInstruction instr{};
                instr.opCode = IL::OpCode::Load;
                instr.result = result;
                instr.source = IL::Source::Invalid();
                instr.address = table.idMap.GetMappedRelative(anchor, reader.ConsumeOp());
                basicBlock->Append(instr);
                break;
            }

            case LLVMFunctionRecord::InstStore: {
                // Append
                IL::StoreInstruction instr{};
                instr.opCode = IL::OpCode::Store;
                instr.result = result;
                instr.source = IL::Source::Invalid();
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
                instr.source = IL::Source::Invalid();
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
                uint64_t calledType = reader.ConsumeOp();

                // Get call declaration
                const DXILFunctionDeclaration* callDecl = GetFunctionDeclaration(called);

                // Create mapping if present
                if (!callDecl->type->returnType->Is<Backend::IL::VoidType>()) {
                    result = table.idMap.AllocMappedID(DXILIDType::Instruction);
                }

                // Try intrinsic
                if (!TryParseIntrinsic(basicBlock, reader, anchor, called, callDecl)) {
                    // Unknown, emit as unexposed
                    IL::UnexposedInstruction instr{};
                    instr.opCode = IL::OpCode::Unexposed;
                    instr.result = result;
                    instr.source = IL::Source::Invalid();
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
    }
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
            return false;
        case LLVMFunctionRecord::InstCall:
        case LLVMFunctionRecord::InstCall2:
            // Handle in call
            return false;
        case LLVMFunctionRecord::InstStore2:
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

void DXILPhysicalBlockFunction::ParseModuleFunction(const struct LLVMRecord &record) {
    LLVMRecordReader reader(record);

    /*
     * LLVM Specification
     *   [FUNCTION, type, callingconv, isproto,
     *    linkage, paramattr, alignment, section, visibility, gc, prologuedata,
     *     dllstorageclass, comdat, prefixdata, personalityfn, preemptionspecifier]
     */

    // Allocate id to current function offset
    table.idMap.AllocMappedID(DXILIDType::Function, functions.Size());

    // Create function
    DXILFunctionDeclaration &function = functions.Add();

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

bool DXILPhysicalBlockFunction::TryParseIntrinsic(IL::BasicBlock* basicBlock, LLVMRecordReader &reader, uint32_t anchor, uint32_t called, const DXILFunctionDeclaration* declaration) {
    LLVMRecordStringView view = table.symbol.GetValueString(called);

    // Check hash
    switch (view.GetHash()) {
        default: {
            // Not an intrinsic
            return false;
        }

        /*
         * LLVM Specification
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

            // Unknown, emit as unexposed
            IL::StoreOutputInstruction instr{};
            instr.opCode = IL::OpCode::StoreOutput;
            instr.result = IL::InvalidID;
            instr.source = IL::Source::Invalid();
            instr.index = outputID;
            instr.row = row;
            instr.column = column;
            instr.value = value;
            basicBlock->Append(instr);
            return true;
        }
    }
}
