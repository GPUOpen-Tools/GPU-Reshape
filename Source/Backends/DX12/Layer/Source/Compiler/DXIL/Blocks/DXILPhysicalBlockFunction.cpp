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

#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockFunction.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXIL/Intrinsic/DXILIntrinsics.Gen.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordReader.h>
#include <Backends/DX12/Compiler/DXIL/DXILValueReader.h>
#include <Backends/DX12/Compiler/DXIL/DXILValueWriter.h>
#include <Backends/DX12/Compiler/DXIL/DXILIDMap.h>
#include <Backends/DX12/Compiler/DXIL/DXIL.Gen.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBitStreamReader.h>
#include <Backends/DX12/Compiler/Tags.h>
#include <Backends/DX12/Compiler/DXCompileJob.h>
#include <Backends/DX12/Resource/VirtualResourceMapping.h>
#include <Backends/DX12/Resource/DescriptorData.h>

// Backend
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/PrettyPrint.h>
#include <Backend/IL/Constant.h>
#include <Backend/IL/ID.h>
#include <Backend/IL/ResourceTokenPacking.h>
#include <Backend/IL/Instruction.h>
#include <Backend/IL/Type.h>
#include <Backend/IL/ResourceTokenType.h>
#include <Backend/IL/TypeSize.h>

// Common
#include <Common/Sink.h>
#include <Common/Containers/TrivialStackVector.h>

// Std
#ifndef NDEBUG
#include <sstream>
#endif // NDEBUG

/*
 * LLVM DXIL Specification
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/DXIL.rst
 *
 * Loosely derived from LLVM BitcodeWriter
 *   https://github.com/microsoft/DirectXShaderCompiler/blob/main/lib/Bitcode/Writer/BitcodeWriter.cpp
 */

DXILPhysicalBlockFunction::DXILPhysicalBlockFunction(const Allocators &allocators, Backend::IL::Program &program, DXILPhysicalBlockTable &table):
    DXILPhysicalBlockSection(allocators, program, table),
    sourceTraceback(allocators) {
    /* */
}

void DXILPhysicalBlockFunction::ParseFunction(struct LLVMBlock *block) {
    // Definition order is linear to the internally linked functions
    uint32_t linkedIndex = internalLinkedFunctions[program.GetFunctionList().GetCount()];

    // Get function definition
    DXILFunctionDeclaration *declaration = functions[linkedIndex];

    // Create snapshot
    DXILIDMap::Snapshot idMapSnapshot = table.idMap.CreateSnapshot();

    // Get type map
    Backend::IL::TypeMap &ilTypeMap = program.GetTypeMap();

    // Create function
    ASSERT(table.metadata.entryPointId != IL::InvalidID, "Expected entrypoint");
    IL::Function *fn = program.GetFunctionList().AllocFunction(table.metadata.entryPointId);

    // Set the type
    fn->SetFunctionType(declaration->type);

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
    for (uint32_t i = 0; i < declaration->type->parameterTypes.size(); i++) {
        declaration->parameters.Add(table.idMap.GetAnchor());
        table.idMap.AllocMappedID(DXILIDType::Parameter);
    }

    // Allocate basic block
    IL::BasicBlock *basicBlock = fn->GetBasicBlocks().AllocBlock();

    // Local block mappings
    TrivialStackVector<IL::BasicBlock *, 32> blockMapping(allocators);
    blockMapping.Add(basicBlock);

    // Current block index
    uint32_t blockIndex{0};

    // Reserve forward allocations
    table.idMap.ReserveForward(block->records.size());

    // Reserve source traceback
    sourceTraceback.resize(block->records.size());

    // Visit function records
    for (uint32_t recordIdx = 0; recordIdx < static_cast<uint32_t>(block->records.size()); recordIdx++) {
        LLVMRecord &record = block->records[recordIdx];

        // Setup reader
        DXILValueReader reader(table, record);

        // Provide traceback
        if (basicBlock != nullptr) {
            sourceTraceback[recordIdx] = DXCodeOffsetTraceback {
                .basicBlockID = basicBlock->GetID(),
                .instructionIndex = basicBlock->GetCount()
            };
        }
        
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
                const uint32_t blockCount = reader.ConsumeOp32();

                // Allocate all blocks (except entry)
                for (uint32_t i = 0; i < blockCount - 1; i++) {
                    IL::ID id = program.GetIdentifierMap().AllocID();
                    blockMapping.Add(fn->GetBasicBlocks().AllocBlock(id));
                }
                break;
            }
            case LLVMFunctionRecord::InstBinOp: {
                uint32_t lhs = reader.GetMappedRelativeValue(anchor);
                uint32_t rhs = reader.GetMappedRelative(anchor);

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
                uint32_t value = reader.GetMappedRelativeValue(anchor);

                // Create type mapping
                ilTypeMap.SetType(result, table.type.typeMap.GetType(reader.ConsumeOp32()));

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
                        instr.symbol = "LLVMCastOp";
                        instr.traits.foldableWithImmediates = true;
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

            case LLVMFunctionRecord::InstSelect: {
                ASSERT(false, "Unsupported instruction");
                break;
            }

            case LLVMFunctionRecord::InstInsertELT: {
                ASSERT(false, "Untested path, validate and remove");

                // Get composite
                const Backend::IL::Type* compositeType = table.type.typeMap.GetType(reader.ConsumeOp32());
                uint32_t compositeValue = reader.GetMappedRelative(anchor);

                // Get index
                const Backend::IL::Type* indexType = table.type.typeMap.GetType(reader.ConsumeOp32());
                uint32_t indexValue = reader.GetMappedRelative(anchor);

                // Unused
                GRS_SINK(compositeType);
                GRS_SINK(indexType);

                IL::InsertInstruction instr{};
                instr.opCode = IL::OpCode::Insert;
                instr.result = result;
                instr.source = IL::Source::Code(recordIdx);
                instr.composite = compositeValue;
                instr.value = indexValue;
                basicBlock->Append(instr);
                break;
            }

            case LLVMFunctionRecord::InstExtractELT: {
                ASSERT(false, "Untested path, validate and remove");

                // Get composite
                const Backend::IL::Type* compositeType = table.type.typeMap.GetType(reader.ConsumeOp32());
                uint32_t compositeValue = reader.GetMappedRelative(anchor);

                // Get index
                uint32_t indexValue = reader.GetMappedRelative(anchor);

                // Unused
                GRS_SINK(compositeType);

                IL::ExtractInstruction instr{};
                instr.opCode = IL::OpCode::Extract;
                instr.result = result;
                instr.source = IL::Source::Code(recordIdx);
                instr.composite = compositeValue;
                instr.index = program.GetConstants().UInt(indexValue)->id;
                basicBlock->Append(instr);
                break;
            }

            case LLVMFunctionRecord::InstExtractVal: {
                // Get composite
                uint32_t compositeValue = reader.GetMappedRelativeValue(anchor);

                // Get index, not relative
                uint32_t index = reader.ConsumeOp32();

                // Create type mapping
                const Backend::IL::Type* type = ilTypeMap.GetType(compositeValue);
                switch (type->kind) {
                    default:
                        ASSERT(false, "Invalid composite extraction");
                        break;
                    case Backend::IL::TypeKind::Struct:
                        ilTypeMap.SetType(result, type->As<Backend::IL::StructType>()->memberTypes[index]);
                        break;
                    case Backend::IL::TypeKind::Vector:
                        ilTypeMap.SetType(result, type->As<Backend::IL::VectorType>()->containedType);
                        break;
                }

                // While LLVM supports this, DXC, given the scalarized nature, does not make use of it
                ASSERT(!reader.Any(), "Unexpected extraction count on InstExtractVal");

                // Create extraction instruction
                IL::ExtractInstruction instr{};
                instr.opCode = IL::OpCode::Extract;
                instr.result = result;
                instr.source = IL::Source::Code(recordIdx);
                instr.composite = compositeValue;
                instr.index = program.GetConstants().UInt(index)->id;

                basicBlock->Append(instr);
                break;
            }

            /* Vectorized instruction not used */
            case LLVMFunctionRecord::InstInsertVal: {
                ASSERT(false, "Unsupported instruction");
                break;
            }

                /* Structural */
            case LLVMFunctionRecord::InstGEP:
            case LLVMFunctionRecord::InstInBoundsGEP: {
                bool inBounds{false};

                // The current pointee type
                const Backend::IL::Type* pointee{ nullptr };

                // Handle old instruction types
                if (record.Is(LLVMFunctionRecord::InstGEP)) {
                    inBounds = reader.ConsumeOpAs<bool>();
                    pointee = table.type.typeMap.GetType(reader.ConsumeOp32());
                } else if (record.Is(LLVMFunctionRecord::InstGEPOld)) {
                    inBounds = true;
                }

                // Get first chain
                uint32_t compositeId = reader.GetMappedRelativeValue(anchor);

                // Get type of composite if needed
                const auto* elementType = ilTypeMap.GetType(compositeId);

                // Number of address cases
                const uint32_t addressCount = reader.Remaining();

                // Unused
                GRS_SINK(pointee, inBounds);

                // Allocate instruction
                auto *instr = ALLOCA_SIZE(IL::AddressChainInstruction, IL::AddressChainInstruction::GetSize(addressCount));
                instr->opCode = IL::OpCode::AddressChain;
                instr->result = result;
                instr->source = IL::Source::Code(recordIdx);
                instr->composite = compositeId;
                instr->chains.count = addressCount;

                for (uint32_t i = 0; i < addressCount; i++) {
                    // Get next chain
                    uint32_t nextChainId = reader.GetMappedRelativeValue(anchor);

                    // Constant indexing into struct?
                    switch (elementType->kind) {
                        default:
                            ASSERT(false, "Unexpected GEP chain type");
                            break;
                        case Backend::IL::TypeKind::None:
                            break;
                        case Backend::IL::TypeKind::Vector: {
                            elementType = elementType->As<Backend::IL::VectorType>()->containedType;
                            break;
                        }
                        case Backend::IL::TypeKind::Matrix: {
                            elementType = elementType->As<Backend::IL::MatrixType>()->containedType;
                            break;
                        }
                        case Backend::IL::TypeKind::Pointer:{
                            elementType = elementType->As<Backend::IL::PointerType>()->pointee;
                            break;
                        }
                        case Backend::IL::TypeKind::Array:{
                            elementType = elementType->As<Backend::IL::ArrayType>()->elementType;
                            break;
                        }
                        case Backend::IL::TypeKind::Struct: {
                            const Backend::IL::Constant* constant = program.GetConstants().GetConstant(nextChainId);
                            ASSERT(constant, "GEP struct chains must be constant");

                            uint32_t memberIdx = static_cast<uint32_t>(constant->As<Backend::IL::IntConstant>()->value);
                            elementType = elementType->As<Backend::IL::StructType>()->memberTypes[memberIdx];
                            break;
                        }
                    }

                    // Set index
                    instr->chains[i].index = nextChainId;
                }

                // Set the resulting type as a pointer to the walked type
                ilTypeMap.SetType(instr->result, ilTypeMap.FindTypeOrAdd(Backend::IL::PointerType {
                    .pointee = elementType,
                    .addressSpace = Backend::IL::AddressSpace::Function
                }));

                basicBlock->Append(instr);
                break;
            }

                /* Select */
            case LLVMFunctionRecord::InstVSelect: {
                uint32_t pass = reader.GetMappedRelativeValue(anchor);
                uint32_t fail = reader.GetMappedRelative(anchor);
                uint32_t condition = reader.GetMappedRelativeValue(anchor);

                // Create type mapping
                ilTypeMap.SetType(result, ilTypeMap.GetType(pass));

                // Emit as select
                IL::SelectInstruction instr{};
                instr.opCode = IL::OpCode::Select;
                instr.result = result;
                instr.source = IL::Source::Code(recordIdx);
                instr.condition = condition;
                instr.pass = pass;
                instr.fail = fail;
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
                instr.symbol = "LLVMShuffle";
                instr.traits.foldableWithImmediates = true;
                basicBlock->Append(instr);
                break;
            }

            case LLVMFunctionRecord::InstCmp:
            case LLVMFunctionRecord::InstCmp2: {
                uint32_t lhs = reader.GetMappedRelativeValue(anchor);
                uint32_t rhs = reader.GetMappedRelative(anchor);

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
                        instr.symbol = "LLVMCmpOp";
                        instr.traits.foldableWithImmediates = true;
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

            case LLVMFunctionRecord::InstAtomicRW: {
                uint32_t address = reader.GetMappedRelativeValue(anchor);
                uint32_t value = reader.GetMappedRelative(anchor);
                uint64_t op = reader.ConsumeOp();
                uint64_t _volatile = reader.ConsumeOp();
                uint64_t ordering = reader.ConsumeOp();
                uint64_t scope = reader.ConsumeOp();

                // Unused
                GRS_SINK(value);
                GRS_SINK(op);
                GRS_SINK(_volatile);
                GRS_SINK(ordering);
                GRS_SINK(scope);

                const auto* pointerType = ilTypeMap.GetType(address)->As<Backend::IL::PointerType>();

                // Emit as unexposed
                IL::UnexposedInstruction instr{};
                instr.opCode = IL::OpCode::Unexposed;
                instr.result = result;
                instr.source = IL::Source::Code(recordIdx);
                instr.backendOpCode = record.id;
                instr.symbol = "AtomicRW";
                basicBlock->Append(instr);

                // Set resulting type
                ilTypeMap.SetType(result, pointerType->pointee);
                break;
            }

            case LLVMFunctionRecord::InstRet: {
                // Emit as unexposed
                IL::ReturnInstruction instr{};
                instr.opCode = IL::OpCode::Return;
                instr.result = result;
                instr.source = IL::Source::Code(recordIdx);

                // Mapping
                if (reader.Any()) {
                    instr.value = reader.GetMappedRelativeValue(anchor);
                } else {
                    instr.value = IL::InvalidID;
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
                uint32_t pass = blockMapping[reader.ConsumeOp()]->GetID();

                // Conditional?
                if (reader.Any()) {
                    IL::BranchConditionalInstruction instr{};
                    instr.opCode = IL::OpCode::BranchConditional;
                    instr.result = result;
                    instr.source = IL::Source::Code(recordIdx);
                    instr.pass = pass;
                    instr.fail = blockMapping[reader.ConsumeOp()]->GetID();
                    instr.cond = reader.GetMappedRelative(anchor);
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

                uint32_t value = reader.GetMappedRelative(anchor);
                uint32_t _default = blockMapping[reader.ConsumeOp()]->GetID();

                // Get remaining count
                uint32_t remaining = reader.Remaining();
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
                    _case.literal = table.idMap.GetMapped(reader.ConsumeOp());
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
                instr.symbol = "Unreachable";
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
                ilTypeMap.SetType(result, table.type.typeMap.GetType(reader.ConsumeOp32()));

                // Get remaining count
                uint32_t remaining = reader.Remaining();
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
                const Backend::IL::Type *type = table.type.typeMap.GetType(reader.ConsumeOp32());

                // Create type mapping
                ilTypeMap.SetType(result, ilTypeMap.FindTypeOrAdd(Backend::IL::PointerType{
                    .pointee = type,
                    .addressSpace = Backend::IL::AddressSpace::Function
                }));

                uint64_t sizeType = reader.ConsumeOp();
                uint64_t size = reader.ConsumeOp();

                // Unused
                GRS_SINK(sizeType);
                GRS_SINK(size);

                // Append
                IL::AllocaInstruction instr{};
                instr.opCode = IL::OpCode::Alloca;
                instr.result = result;
                instr.source = IL::Source::Code(recordIdx);
                basicBlock->Append(instr);
                break;
            }

            case LLVMFunctionRecord::InstLoad: {
                uint32_t address = reader.GetMappedRelativeValue(anchor);

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
                instr.address = reader.GetMappedRelativeValue(anchor);

                instr.value = reader.GetMappedRelative(anchor);
                basicBlock->Append(instr);
                break;
            }
            case LLVMFunctionRecord::InstStore2: {
                // Append
                IL::StoreInstruction instr{};
                instr.opCode = IL::OpCode::Store;
                instr.result = result;
                instr.source = IL::Source::Code(recordIdx);
                instr.address = reader.GetMappedRelativeValue(anchor);

                // Type
                reader.ConsumeOp();

                instr.value = reader.GetMappedRelative(anchor);
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

                // Unused
                GRS_SINK(attributes);
                GRS_SINK(callConv);
                GRS_SINK(isTailCall);
                GRS_SINK(isMustTailCall);
                GRS_SINK(type);

                // Get callee
                uint32_t called = table.idMap.GetRelative(anchor, reader.ConsumeOp32());

                // Get call declaration
                const DXILFunctionDeclaration *callDecl = GetFunctionDeclaration(called);

                // Mark the declaration as used
                table.compliance.MarkAsUsed(callDecl);

                // Create mapping if present
                if (!callDecl->type->returnType->Is<Backend::IL::VoidType>()) {
                    result = table.idMap.AllocMappedID(DXILIDType::Instruction);

                    // Set as return type
                    ilTypeMap.SetType(result, callDecl->type->returnType);
                }

                // Function parameters may be forward declared without immediate types, so map it out immediately
                for (uint32_t i = 0; i < callDecl->type->parameterTypes.size(); i++) {
                    uint32_t id = reader.record.Op32(reader.Offset() + i);

                    // If resolved, skip it
                    if (table.idMap.IsResolved(anchor, id)) {
                        continue;
                    }

                    // Get the forward value
                    IL::ID linear = table.idMap.GetMappedForward(anchor, DXILIDRemapper::DecodeForward(id));

                    // Assign forward type from declaration
                    program.GetTypeMap().SetType(linear, callDecl->type->parameterTypes[i]);
                }

                // General unexposed in case the instruction is unknown
                IL::UnexposedInstruction unexposed{};

                // Try intrinsic
                if (!TryParseIntrinsic(basicBlock, recordIdx, reader, anchor, called, result, unexposed)) {
                    // Unknown, emit as unexposed
                    unexposed.opCode = IL::OpCode::Unexposed;
                    unexposed.result = result;
                    unexposed.source = IL::Source::Code(recordIdx);
                    unexposed.backendOpCode = record.id;
                    unexposed.symbol = table.symbol.GetValueAllocation(called);
                    basicBlock->Append(unexposed);
                }
                break;
            }

                /* Debug */
            case LLVMFunctionRecord::DebugLOC:
            case LLVMFunctionRecord::DebugLOCAgain:
            case LLVMFunctionRecord::DebugLOC2: {
                // Handled in non-canonical ILDB path
                break;
            }
        }

        // Set result
        record.SetSource(result != IL::InvalidID, anchor);
    }

    // Validation
    ASSERT(blockIndex == blockMapping.Size(), "Terminator to block count mismatch");

    // Validation
#ifndef NDEBUG
    for (const IL::BasicBlock *fnBB: fn->GetBasicBlocks()) {
        for (const IL::Instruction *instr: *fnBB) {
            if (instr->result == IL::InvalidID) {
                continue;
            }

            // Unexposed instructions are exempt
            if (instr->Is<IL::UnexposedInstruction>()) {
                continue;
            }

            // While the instructions themselves can be unexposed, the resulting type must never be.
            // Supporting this from the user side would be needless complexity
            const Backend::IL::Type* type = ilTypeMap.GetType(instr->result);
            if (!type || type->kind == Backend::IL::TypeKind::Unexposed)
            {
                // Compose message
                std::stringstream stream;
                stream << "Instruction with unexposed results are invalid\n\t";
                IL::PrettyPrint(&program, instr, stream);

                // Panic!
                ASSERT(false, stream.str().c_str());
            }

            // Debugger hack to keep instr in the scope
            bool _ = false;
            GRS_SINK(_);
        }
    }
#endif // NDEBUG

    // Only create value segments if there's more than one function, no need to branch if not
    if (RequiresValueMapSegmentation()) {
        // Create id map segment
        declaration->segments.idSegment = table.idMap.Branch(idMapSnapshot);
    }
}

void DXILPhysicalBlockFunction::MigrateConstantBlocks() {
    LLVMBlock& root = table.scan.GetRoot();

    /*
     * Migrate all in-function constants to global constant map due to an
     * LLVM bug with metadata value forward references. The LLVM bit-decoder
     * reallocates the value lookup map to the forward bound, however, sets the initial
     * value index during function reading to the array bound, not the *current* value bound.
     */

    // Function counter
    uint32_t functionIndex = 0;

    // For all functions
    for (LLVMBlock* block : root.blocks) {
        if (static_cast<LLVMReservedBlock>(block->id) != LLVMReservedBlock::Function) {
            continue;
        }

        // Definition order is linear to the internally linked functions
        uint32_t linkedIndex = internalLinkedFunctions[functionIndex++];

        // Get function definition
        DXILFunctionDeclaration *declaration = functions[linkedIndex];

        // Constant offset
        uint32_t constantOffset = 0;

        // Move all constant data
        for (LLVMBlock *fnBlock: block->blocks) {
            switch (static_cast<LLVMReservedBlock>(fnBlock->id)) {
                default: {
                    break;
                }
                case LLVMReservedBlock::Constants: {
                    // Get the destination migration block
                    LLVMBlock* migrationBlock = table.scan.GetRoot().GetBlock(LLVMReservedBlock::Constants);

                    // Move all records
                    for (const LLVMBlockElement& element : fnBlock->elements) {
                        if (element.Is(LLVMBlockElementType::Record)) {
                            LLVMRecord record = fnBlock->records[element.id];

                            // Remove abbreviation
                            //   All records are unabbreviated at this point, abbreviations may be block local which is
                            //   unsafe after moving. Always assume unabbreviated exports.
                            record.abbreviation.type = LLVMRecordAbbreviationType::None;

                            // Handle segmentation remapping if needed
                            if (RequiresValueMapSegmentation() && record.hasValue) {
                                // Not expecting user data right now
                                ASSERT(record.result == IL::InvalidID, "Unexpected record state");

                                // Get the original state
                                const DXILIDMap::NativeState& state = declaration->segments.idSegment.map[constantOffset++];
                                ASSERT(state.type == DXILIDType::Constant, "Unexpected native state");

                                // Add for later value remapping
                                declaration->segments.constantRelocationTable.push_back(DXILFunctionConstantRelocation {
                                    .sourceAnchor = record.sourceAnchor,
                                    .mapped = state.mapped
                                });

                                // Handle operands
                                switch (static_cast<LLVMConstantRecord>(record.id)) {
                                    default: {
                                        break;
                                    }
                                    case LLVMConstantRecord::Aggregate: {
                                        for (uint32_t i = 0; i < record.opCount; i++) {
                                            MigrateConstantBlockOperand(declaration, record.Op(i));
                                        }
                                        break;
                                    }
                                    case LLVMConstantRecord::InBoundsGEP: {
                                        for (uint32_t i = 1; i < record.opCount; i += 2) {
                                            MigrateConstantBlockOperand(declaration, record.Op(i + 1));
                                        }
                                        break;
                                    }
                                    case LLVMConstantRecord::Cast: {
                                        MigrateConstantBlockOperand(declaration, record.Op(2));
                                        break;
                                    }
                                }

                                // Mark record as "user", value stitched to the user IL id
                                record.SetUser(true, ~0u, state.mapped);
                            }

                            // Add to new block
                            migrationBlock->AddRecord(record);
                        }
                    }

                    // Flush block
                    fnBlock->elements.resize(0);
                    fnBlock->records.resize(0);
                    break;
                }
            }
        }
    }
}

void DXILPhysicalBlockFunction::MigrateConstantBlockOperand(DXILFunctionDeclaration* declaration, uint64_t& operand) {
    // Head offset before segmentation
    uint64_t mapOffset = declaration->segments.idSegment.head.mapOffset;
    
    // Get the original state
    uint32_t mappedId;
    if (operand >= mapOffset) {
        // Get native state and validate
        const DXILIDMap::NativeState& opState = declaration->segments.idSegment.map[operand - mapOffset];
        ASSERT(opState.type == DXILIDType::Constant, "Unexpected native state");

        // Assume mapped
        mappedId = opState.mapped;
    } else {
        mappedId = table.idMap.GetMappedCheckType(operand, DXILIDType::Constant);
    }

    // Remap
    operand = DXILIDRemapper::EncodeUserOperand(mappedId);
}

bool DXILPhysicalBlockFunction::HasResult(const struct LLVMRecord &record) {
    return HasValueAllocation(record.As<LLVMFunctionRecord>(), record.opCount);
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

    IL::ID id = table.idMap.AllocMappedID(DXILIDType::Function, static_cast<uint32_t>(functions.Size()));

    // Create function
    auto *function = functions.Add(new (allocators, kAllocModuleDXIL) DXILFunctionDeclaration(allocators));

    // Set id
    function->anchor = record.sourceAnchor;
    function->id = DXILIDRemapper::EncodeUserOperand(id);

    // Hash name
    function->hash = std::hash<std::string_view>{}(function->name);

    // Get function type
    uint32_t type = reader.ConsumeOp32();
    function->type = table.type.typeMap.GetType(type)->As<Backend::IL::FunctionType>();

    // Ignored
    uint64_t callingConv = reader.ConsumeOp();
    function->isPrototype = reader.ConsumeOp();

    // Get function linkage
    function->linkage = static_cast<LLVMLinkage>(reader.ConsumeOp());

    // Ignored
    uint64_t paramAttr = reader.ConsumeOp();

    // Unused
    GRS_SINK(callingConv);
    GRS_SINK(paramAttr);

    // Add to internal linked functions if not external
    if (!function->isPrototype) {
        internalLinkedFunctions.Add(static_cast<uint32_t>(functions.Size() - 1));
    }
}

const DXILFunctionDeclaration *DXILPhysicalBlockFunction::GetFunctionDeclaration(uint32_t id) {
    ASSERT(table.idMap.GetType(id) == DXILIDType::Function, "Invalid function id");
    return functions[table.idMap.GetDataIndex(id)];
}

const DXILFunctionDeclaration * DXILPhysicalBlockFunction::GetFunctionDeclarationFromIndex(uint32_t index) {
    return functions[index];
}

DXCodeOffsetTraceback DXILPhysicalBlockFunction::GetCodeOffsetTraceback(uint32_t codeOffset) {
    if (codeOffset >= sourceTraceback.size()) {
        return sourceTraceback.back();
    }
    
    return sourceTraceback.at(codeOffset);
}

static void PopulateUnexposedInstructionTraits(DXILOpcodes opCode, IL::UnexposedInstructionTraits& traits) {
    switch (opCode) {
        default:
            break;
        case DXILOpcodes::FAbs_:
        case DXILOpcodes::Saturate_:
        case DXILOpcodes::Cos_:
        case DXILOpcodes::Sin_:
        case DXILOpcodes::Tan_:
        case DXILOpcodes::Acos_:
        case DXILOpcodes::Asin_:
        case DXILOpcodes::Atan_:
        case DXILOpcodes::Hcos_:
        case DXILOpcodes::Hsin_:
        case DXILOpcodes::Htan_:
        case DXILOpcodes::Exp_:
        case DXILOpcodes::Frc_:
        case DXILOpcodes::Log_:
        case DXILOpcodes::Sqrt_:
        case DXILOpcodes::Rsqrt_:
        case DXILOpcodes::Round_ne_:
        case DXILOpcodes::Round_ni_:
        case DXILOpcodes::Round_pi_:
        case DXILOpcodes::Round_z_:
        case DXILOpcodes::FMax_:
        case DXILOpcodes::FMin_:
        case DXILOpcodes::FMad_:
        case DXILOpcodes::Fma_:
        case DXILOpcodes::Dot2_:
        case DXILOpcodes::Dot3_:
        case DXILOpcodes::Dot4_:
        case DXILOpcodes::Bfrev_:
        case DXILOpcodes::Countbits_:
        case DXILOpcodes::FirstbitLo_:
        case DXILOpcodes::FirstbitHi_:
        case DXILOpcodes::FirstbitSHi_:
        case DXILOpcodes::IMin_:
        case DXILOpcodes::IMax_:
        case DXILOpcodes::UMin_:
        case DXILOpcodes::UMax_:
        case DXILOpcodes::IMad_:
        case DXILOpcodes::UMad_:
        case DXILOpcodes::Ubfe_:
        case DXILOpcodes::Ibfe_:
        case DXILOpcodes::Bfi_: {
            traits.foldableWithImmediates = true;
            break;
        }
        case DXILOpcodes::AtomicBinOp:
        case DXILOpcodes::AtomicCompareExchange:
        case DXILOpcodes::LoadInput_:
        case DXILOpcodes::BufferUpdateCounter:
        case DXILOpcodes::CycleCounterLegacy:
        case DXILOpcodes::DomainLocation:
        case DXILOpcodes::Coverage:
        case DXILOpcodes::EvalCentroid:
        case DXILOpcodes::EvalSampleIndex:
        case DXILOpcodes::EvalSnapped:
        case DXILOpcodes::FlattenedThreadIdInGroup:
        case DXILOpcodes::GSInstanceID:
        case DXILOpcodes::InnerCoverage:
        case DXILOpcodes::LoadOutputControlPoint:
        case DXILOpcodes::LoadPatchConstant:
        case DXILOpcodes::OutputControlPointID:
        case DXILOpcodes::PrimitiveID:
        case DXILOpcodes::RenderTargetGetSampleCount:
        case DXILOpcodes::RenderTargetGetSamplePosition:
        case DXILOpcodes::ThreadId:
        case DXILOpcodes::ThreadIdInGroup: {
            traits.divergent = true;
            break;
        }
    }
}

bool DXILPhysicalBlockFunction::TryParseIntrinsic(IL::BasicBlock *basicBlock, uint32_t recordIdx, DXILValueReader &reader, uint32_t anchor, uint32_t called, uint32_t result, IL::UnexposedInstruction& unexposed) {
    LLVMRecordStringView view = table.symbol.GetValueString(called);

    // Get type map
    Backend::IL::TypeMap& ilTypeMap = program.GetTypeMap();

    // Must match, if it needs to deviate then do translation instead
    static_assert(static_cast<uint32_t>(IL::ComponentMask::X) == BIT(0), "Unexpected color mask");
    static_assert(static_cast<uint32_t>(IL::ComponentMask::Y) == BIT(1), "Unexpected color mask");
    static_assert(static_cast<uint32_t>(IL::ComponentMask::Z) == BIT(2), "Unexpected color mask");
    static_assert(static_cast<uint32_t>(IL::ComponentMask::W) == BIT(3), "Unexpected color mask");

    // If not an intrinsic, not interested
    if (!view.StartsWith("dx.op.")) {
        return false;
    }

    // Get op code
    auto opCode = static_cast<DXILOpcodes>(program.GetConstants().GetConstant<IL::IntConstant>(reader.GetMappedRelative(anchor))->value);

    // Parse as intrinsic
    switch (opCode) {
        default: {
            // If a reserved operation, populate the traits
            PopulateUnexposedInstructionTraits(opCode, unexposed.traits);

            // Allocate operands
            unexposed.operandCount = reader.Remaining();
            unexposed.operands = table.recordAllocator.AllocateArray<IL::ID>(unexposed.operandCount);

            // Read operands
            for (uint32_t i = 0; i < unexposed.operandCount; i++) {
                unexposed.operands[i] = reader.GetMappedRelative(anchor);
            }

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

        case DXILOpcodes::CreateHandle: {
            // Resource class
            auto _class = static_cast<DXILShaderResourceClass>(program.GetConstants().GetConstant<IL::IntConstant>(reader.GetMappedRelative(anchor))->value);

            // Handle ids are always stored as constants
            uint32_t handleId = static_cast<uint32_t>(program.GetConstants().GetConstant<IL::IntConstant>(reader.GetMappedRelative(anchor))->value);

            // Range indices may be dynamic
            IL::ID rangeIndex = reader.GetMappedRelative(anchor);

            // Divergent?
            auto isNonUniform = program.GetConstants().GetConstant<IL::BoolConstant>(reader.GetMappedRelative(anchor))->value;

            // Get the actual handle type
            auto type = table.metadata.GetHandleType(_class, handleId);

            // Unexposed remote address
            IL::ID bindingGroup = table.metadata.GetTypeSymbolicBindingGroup(type);
            IL::ID chainAddr = program.GetIdentifierMap().AllocID();
            
            // Represent indexing through a symbolic address chain instruction
            auto *chain = ALLOCA_SIZE(IL::AddressChainInstruction, IL::AddressChainInstruction::GetSize(2u));
            chain->opCode = IL::OpCode::AddressChain;
            chain->result = chainAddr;
            chain->source = IL::Source::Symbolic(recordIdx);
            chain->composite = bindingGroup;
            chain->chains.count = 2;
            chain->chains[0].index = program.GetConstants().UInt(0)->id;
            chain->chains[1].index = rangeIndex;
            basicBlock->Append(chain);

            // Set type to Handle*
            ilTypeMap.SetType(chainAddr, ilTypeMap.FindTypeOrAdd(Backend::IL::PointerType {
                .pointee = type,
                .addressSpace = ilTypeMap.GetType(bindingGroup)->As<Backend::IL::PointerType>()->addressSpace
            }));
            
            // Expose actual handle as a load
            IL::LoadInstruction load{};
            load.opCode = IL::OpCode::Load;
            load.result = result;
            load.source = IL::Source::Code(recordIdx);
            load.address = chainAddr;
            basicBlock->Append(load);

            // Set type to Handle
            ilTypeMap.SetType(result, type);

            // If non uniform, add the metadata
            if (isNonUniform) {
                IL::MetadataMap& metadata = program.GetMetadataMap();
                metadata.AddMetadata(chainAddr, IL::MetadataType::DivergentResourceIndex);
                metadata.AddMetadata(result, IL::MetadataType::DivergentResourceIndex);
            }
            
            return true;
        }

        /** SM6.6 binding */
        case DXILOpcodes::CreateHandleFromBinding: {
            // Get binding
            auto bindings = program.GetConstants().GetConstant(reader.GetMappedRelative(anchor));

            // Get binding operands
            int64_t rangeLowerBound{0};
            int64_t rangeUpperBound{0};
            int64_t spaceID{0};
            int64_t resourceClass{0};

            // May be null constant
            switch (bindings->kind) {
                default: {
                    ASSERT(false, "Invalid binding kind");
                    break;
                }
                case Backend::IL::ConstantKind::Struct: {
                    auto _struct = bindings->As<IL::StructConstant>();

                    // Get constants
                    ASSERT(_struct->members.size() == 4u, "Unexpected binding size");
                    rangeLowerBound = _struct->members[0]->As<Backend::IL::IntConstant>()->value;
                    rangeUpperBound = _struct->members[1]->As<Backend::IL::IntConstant>()->value;
                    spaceID = _struct->members[2]->As<Backend::IL::IntConstant>()->value;
                    resourceClass = _struct->members[3]->As<Backend::IL::IntConstant>()->value;
                    break;
                }
                case Backend::IL::ConstantKind::Null: {
                    break;
                }
            }

            // Get the actual handle type
            auto type = table.metadata.GetHandle(static_cast<DXILShaderResourceClass>(resourceClass), spaceID, rangeLowerBound, rangeUpperBound);
            
            // Set as pointee type
            ilTypeMap.SetType(result, type->type);

            // Range indices may be dynamic
            IL::ID rangeIndex = reader.GetMappedRelative(anchor);

            // Divergent?
            auto isNonUniform = program.GetConstants().GetConstant<IL::BoolConstant>(reader.GetMappedRelative(anchor))->value;

            // Ignore
            GRS_SINK(rangeIndex, isNonUniform);

            // Keep the original record
            IL::UnexposedInstruction instr{};
            instr.opCode = IL::OpCode::Unexposed;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.symbol = "dx.op.createHandleFromBinding";
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

        case DXILOpcodes::StoreOutput_: {
            uint32_t outputID = reader.GetMappedRelative(anchor);
            uint32_t row = reader.GetMappedRelative(anchor);
            uint32_t column = reader.GetMappedRelative(anchor);
            uint32_t value = reader.GetMappedRelative(anchor);

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

        case DXILOpcodes::BufferLoad: {
            // Get operands, ignore offset for now
            uint32_t resource = reader.GetMappedRelative(anchor);
            uint32_t coordinate = reader.GetMappedRelative(anchor);
            uint32_t offset = reader.GetMappedRelative(anchor);

            // Unused
            GRS_SINK(offset);

            // Emit as load
            IL::LoadBufferInstruction instr{};
            instr.opCode = IL::OpCode::LoadBuffer;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.buffer = resource;
            instr.index = coordinate;
            instr.offset = offset;
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

        case DXILOpcodes::BufferStore: {
            // Get operands, ignore offset for now
            uint32_t resource = reader.GetMappedRelative(anchor);
            uint32_t coordinate = reader.GetMappedRelative(anchor);
            uint32_t offset = reader.GetMappedRelative(anchor);
            uint32_t x = reader.GetMappedRelative(anchor);
            uint32_t y = reader.GetMappedRelative(anchor);
            uint32_t z = reader.GetMappedRelative(anchor);
            uint32_t w = reader.GetMappedRelative(anchor);

            // Get mask
            uint64_t mask = program.GetConstants().GetConstant<IL::IntConstant>(reader.GetMappedRelative(anchor))->value;

            // Unused
            GRS_SINK(offset);

            // Get type
            const auto* bufferType = ilTypeMap.GetType(resource)->As<Backend::IL::BufferType>();

            // Number of dimensions
            uint32_t formatDimensionCount = Backend::IL::GetDimensionSize(bufferType->texelType);

            // Vectorize
            IL::ID svoxValue = AllocateSVOSequential(formatDimensionCount, x, y, z, w);

            // Emit as store
            IL::StoreBufferInstruction instr{};
            instr.opCode = IL::OpCode::StoreBuffer;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.buffer = resource;
            instr.index = coordinate;
            instr.value = svoxValue;
            instr.mask = IL::ComponentMaskSet(mask);
            basicBlock->Append(instr);
            return true;
        }

        /*
         * DXIL Specification
         *   ; overloads: SM5.1: f32|i32,  SM6.0: f32|i32, SM6.2: f16|f32|i16|i32
         *   ; returns: status
         *   declare %dx.types.ResRet.f32 @dx.op.rawBufferLoad.f32(
         *       i32,                  ; opcode
         *       %dx.types.Handle,     ; resource handle
         *       i32,                  ; coordinate c0 (index)
         *       i8,                   ; mask
         *       i32,                  ; alignment
         */

        case DXILOpcodes::RawBufferLoad: {
            // Get operands, ignore offset for now
            uint32_t resource = reader.GetMappedRelative(anchor);
            uint32_t coordinate = reader.GetMappedRelative(anchor);
            uint32_t offset = reader.GetMappedRelative(anchor);
            uint64_t mask = program.GetConstants().GetConstant<IL::IntConstant>(reader.GetMappedRelative(anchor))->value;
            uint64_t alignment = program.GetConstants().GetConstant<IL::IntConstant>(reader.GetMappedRelative(anchor))->value;
            
            // Unused
            GRS_SINK(offset);

            // Emit as load
            IL::LoadBufferRawInstruction instr{};
            instr.opCode = IL::OpCode::LoadBufferRaw;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.buffer = resource;
            instr.index = coordinate;
            instr.offset = offset;
            instr.mask = IL::ComponentMaskSet(mask);
            instr.alignment = static_cast<uint32_t>(alignment);
            basicBlock->Append(instr);
            return true;
        }

        /*
         * DXIL Specification
         *   ; overloads: SM5.1: f32|i32,  SM6.0: f32|i32, SM6.2: f16|f32|i16|i32
         *   declare void @dx.op.rawBufferStore.f32(
         *       i32,                  ; opcode
         *       %dx.types.Handle,     ; resource handle
         *       i32,                  ; coordinate c0 (index)
         *       i32,                  ; coordinate c1 (elementOffset)
         *       float,                ; value v0
         *       float,                ; value v1
         *       float,                ; value v2
         *       float,                ; value v3
         *       i8,                   ; write mask
         *       i32)                  ; alignment
         */

        case DXILOpcodes::RawBufferStore: {
            // Get operands, ignore offset for now
            uint32_t resource = reader.GetMappedRelative(anchor);
            uint32_t coordinate = reader.GetMappedRelative(anchor);
            uint32_t offset = reader.GetMappedRelative(anchor);
            uint32_t x = reader.GetMappedRelative(anchor);
            uint32_t y = reader.GetMappedRelative(anchor);
            uint32_t z = reader.GetMappedRelative(anchor);
            uint32_t w = reader.GetMappedRelative(anchor);

            // Get mask
            uint64_t mask = program.GetConstants().GetConstant<IL::IntConstant>(reader.GetMappedRelative(anchor))->value;
            uint64_t alignment = program.GetConstants().GetConstant<IL::IntConstant>(reader.GetMappedRelative(anchor))->value;

            // Unused
            GRS_SINK(offset);

            // Get type
            const auto* bufferType = ilTypeMap.GetType(resource)->As<Backend::IL::BufferType>();

            // Number of dimensions
            uint32_t formatDimensionCount = Backend::IL::GetDimensionSize(bufferType->texelType);

            // Vectorize
            IL::ID svoxValue = AllocateSVOSequential(formatDimensionCount, x, y, z, w);

            // Emit as store
            IL::StoreBufferRawInstruction instr{};
            instr.opCode = IL::OpCode::StoreBufferRaw;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.buffer = resource;
            instr.index = coordinate;
            instr.value = svoxValue;
            instr.mask = IL::ComponentMaskSet(mask);
            instr.alignment = static_cast<uint32_t>(alignment);
            basicBlock->Append(instr);
            return true;
        }

        /*
         * DXIL Specification
         *  ; overloads: SM5.1: f32|i32,  SM6.0: f16|f32|i16|i32
         *  declare %dx.types.ResRet.f32 @dx.op.textureLoad.f32(
         *      i32,                  ; opcode
         *      %dx.types.Handle,     ; texture handle
         *      i32,                  ; MIP level; sample for Texture2DMS
         *      i32,                  ; coordinate c0
         *      i32,                  ; coordinate c1
         *      i32,                  ; coordinate c2
         *      i32,                  ; offset o0
         *      i32,                  ; offset o1
         *      i32)                  ; offset o2
         */

        case DXILOpcodes::TextureLoad: {
            // Get operands, ignore offset for now
            uint32_t resource = reader.GetMappedRelative(anchor);
            uint32_t mip = reader.GetMappedRelative(anchor);
            uint32_t cx = reader.GetMappedRelative(anchor);
            uint32_t cy = reader.GetMappedRelative(anchor);
            uint32_t cz = reader.GetMappedRelative(anchor);
            uint32_t ox = reader.GetMappedRelative(anchor);
            uint32_t oy = reader.GetMappedRelative(anchor);
            uint32_t oz = reader.GetMappedRelative(anchor);

            // Undef mips are represented as unassigned
            if (const IL::Constant *constant = program.GetConstants().GetConstant(mip); constant && constant->Is<IL::UndefConstant>()) {
                mip = IL::InvalidID;
            }

            // Get type
            const auto* textureType = ilTypeMap.GetType(resource)->As<Backend::IL::TextureType>();

            // Number of dimensions
            uint32_t textureDimensionCount = Backend::IL::GetDimensionSize(textureType->dimension);

            // Vectorize
            IL::ID svoxCoordinate = AllocateSVOSequential(textureDimensionCount, cx, cy, cz);
            IL::ID svoxOffset     = AllocateSVOSequential(textureDimensionCount, ox, oy, oz);

            // Emit as store
            IL::LoadTextureInstruction instr{};
            instr.opCode = IL::OpCode::LoadTexture;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.texture = resource;
            instr.mip = mip;
            instr.offset = svoxOffset;
            instr.index = svoxCoordinate;
            basicBlock->Append(instr);
            return true;
        }

        /*
         * DXIL Specification
         *  declare %dx.types.ResRet.f32 @dx.op.sample.f32(
         *      i32,                      ; opcode
         *      %dx.types.ResHandle,      ; texture handle
         *      %dx.types.SamplerHandle,  ; sampler handle
         *      float,                    ; coordinate c0
         *      float,                    ; coordinate c1
         *      float,                    ; coordinate c2
         *      float,                    ; coordinate c3
         *      i32,                      ; offset o0
         *      i32,                      ; offset o1
         *      i32,                      ; offset o2
         */

        case DXILOpcodes::Sample:
        case DXILOpcodes::SampleBias:
        case DXILOpcodes::SampleLevel:
        case DXILOpcodes::SampleGrad: {
            // Get operands, ignore offset for now
            uint32_t resource = reader.GetMappedRelative(anchor);
            uint32_t sampler = reader.GetMappedRelative(anchor);
            uint32_t cx = reader.GetMappedRelative(anchor);
            uint32_t cy = reader.GetMappedRelative(anchor);
            uint32_t cz = reader.GetMappedRelative(anchor);
            uint32_t cw = reader.GetMappedRelative(anchor);
            uint32_t ox = reader.GetMappedRelative(anchor);
            uint32_t oy = reader.GetMappedRelative(anchor);
            uint32_t oz = reader.GetMappedRelative(anchor);

            // Get type
            const auto* textureType = ilTypeMap.GetType(resource)->As<Backend::IL::TextureType>();

            // Number of dimensions
            uint32_t textureDimensionCount = Backend::IL::GetDimensionSize(textureType->dimension);

            // Vectorize
            IL::ID svoxCoordinate = AllocateSVOSequential(textureDimensionCount, cx, cy, cz, cw);
            IL::ID svoxOffset     = AllocateSVOSequential(textureDimensionCount, ox, oy, oz);

            // Emit as sample
            IL::SampleTextureInstruction instr{};
            instr.opCode = IL::OpCode::SampleTexture;
            instr.sampleMode = Backend::IL::TextureSampleMode::Default;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.texture = resource;
            instr.sampler = sampler;
            instr.coordinate = svoxCoordinate;
            instr.lod = IL::InvalidID;
            instr.bias = IL::InvalidID;
            instr.reference = IL::InvalidID;
            instr.ddx = IL::InvalidID;
            instr.ddy = IL::InvalidID;
            instr.offset = svoxOffset;

            // Handle additional operands
            switch (opCode) {
                default:
                    ASSERT(false, "Unexpected sampling opcode");
                    break;
                case DXILOpcodes::Sample: {
                    instr.sampleMode = Backend::IL::TextureSampleMode::Default;

                    // Unused
                    uint32_t clamp = reader.GetMappedRelative(anchor);
                    GRS_SINK(clamp);
                    break;
                }
                case DXILOpcodes::SampleBias: {
                    instr.sampleMode = Backend::IL::TextureSampleMode::Default;
                    instr.bias = reader.GetMappedRelative(anchor);

                    // Unused
                    uint32_t clamp = reader.GetMappedRelative(anchor);
                    GRS_SINK(clamp);
                    break;
                }
                case DXILOpcodes::SampleCmp: {
                    instr.sampleMode = Backend::IL::TextureSampleMode::DepthComparison;
                    instr.reference = reader.GetMappedRelative(anchor);

                    // Unused
                    uint32_t clamp = reader.GetMappedRelative(anchor);
                    GRS_SINK(clamp);
                    break;
                }
                case DXILOpcodes::SampleCmpLevelZero: {
                    instr.sampleMode = Backend::IL::TextureSampleMode::DepthComparison;
                    instr.reference = reader.GetMappedRelative(anchor);
                    break;
                }
                case DXILOpcodes::SampleGrad: {
                    instr.sampleMode = Backend::IL::TextureSampleMode::Default;

                    // DDX
                    uint32_t ddx0 = reader.GetMappedRelative(anchor);
                    uint32_t ddx1 = reader.GetMappedRelative(anchor);
                    uint32_t ddx2 = reader.GetMappedRelative(anchor);

                    // DDY
                    uint32_t ddy0 = reader.GetMappedRelative(anchor);
                    uint32_t ddy1 = reader.GetMappedRelative(anchor);
                    uint32_t ddy2 = reader.GetMappedRelative(anchor);

                    // Vectorize
                    instr.ddx = AllocateSVOSequential(textureDimensionCount, ddx0, ddx1, ddx2);
                    instr.ddy = AllocateSVOSequential(textureDimensionCount, ddy0, ddy1, ddy2);

                    // Unused
                    uint32_t clamp = reader.GetMappedRelative(anchor);
                    GRS_SINK(clamp);
                    break;
                }
                case DXILOpcodes::SampleLevel: {
                    instr.sampleMode = Backend::IL::TextureSampleMode::Default;
                    instr.lod = reader.GetMappedRelative(anchor);
                    break;
                }
            }

            basicBlock->Append(instr);
            return true;
        }

            /*
             * DXIL Specification
             *   ; overloads: SM5.1: f32|i32,  SM6.0: f16|f32|i16|i32
             *   ; returns: status
             *   declare void @dx.op.textureStore.f32(
             *       i32,                  ; opcode
             *       %dx.types.Handle,     ; texture handle
             *       i32,                  ; coordinate c0
             *       i32,                  ; coordinate c1
             *       i32,                  ; coordinate c2
             *       float,                ; value v0
             *       float,                ; value v1
             *       float,                ; value v2
             *       float,                ; value v3
             *       i8)                   ; write mask
             */

        case DXILOpcodes::TextureStore: {
            // Get operands, ignore offset for now
            uint32_t resource = reader.GetMappedRelative(anchor);
            uint32_t cx = reader.GetMappedRelative(anchor);
            uint32_t cy = reader.GetMappedRelative(anchor);
            uint32_t cz = reader.GetMappedRelative(anchor);
            uint32_t vx = reader.GetMappedRelative(anchor);
            uint32_t vy = reader.GetMappedRelative(anchor);
            uint32_t vz = reader.GetMappedRelative(anchor);
            uint32_t vw = reader.GetMappedRelative(anchor);

            // Get mask
            uint64_t mask = program.GetConstants().GetConstant<IL::IntConstant>(reader.GetMappedRelative(anchor))->value;

            // Get type
            const auto* textureType = ilTypeMap.GetType(resource)->As<Backend::IL::TextureType>();

            // Number of dimensions
            uint32_t textureDimensionCount = Backend::IL::GetDimensionSize(textureType->dimension);
            uint32_t formatDimensionCount = Backend::IL::GetDimensionSize(textureType->format);

            // Vectorize
            IL::ID svoxCoordinate = AllocateSVOSequential(textureDimensionCount, cx, cy, cz);
            IL::ID svoxValue = AllocateSVOSequential(formatDimensionCount, vx, vy, vz, vw);

            // Emit as store
            IL::StoreTextureInstruction instr{};
            instr.opCode = IL::OpCode::StoreTexture;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.texture = resource;
            instr.index = svoxCoordinate;
            instr.texel =  svoxValue;
            instr.mask = IL::ComponentMaskSet(mask);
            basicBlock->Append(instr);
            return true;
        }

        case DXILOpcodes::IsNaN_:
        case DXILOpcodes::IsInf_: {
            // Get operands
            uint32_t value = reader.GetMappedRelative(anchor);

            // Handle op
            switch (opCode) {
                default: {
                    // Unexposed
                    return false;
                }
                case DXILOpcodes::IsNaN_: {
                    // Emit as NaN
                    IL::IsNaNInstruction instr{};
                    instr.opCode = IL::OpCode::IsNaN;
                    instr.result = result;
                    instr.source = IL::Source::Code(recordIdx);
                    instr.value = value;
                    basicBlock->Append(instr);
                    return true;
                }
                case DXILOpcodes::IsInf_: {
                    // Emit as NaN
                    IL::IsInfInstruction instr{};
                    instr.opCode = IL::OpCode::IsInf;
                    instr.result = result;
                    instr.source = IL::Source::Code(recordIdx);
                    instr.value = value;
                    basicBlock->Append(instr);
                    return true;
                }
            }
        }

        case DXILOpcodes::WaveReadLaneFirst: {
            IL::WaveReadFirstInstruction instr{};
            instr.opCode = IL::OpCode::WaveReadFirst;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.value = reader.GetMappedRelative(anchor);
            basicBlock->Append(instr);
            return true;
        }

        case DXILOpcodes::WaveAnyTrue: {
            IL::WaveAnyTrueInstruction instr{};
            instr.opCode = IL::OpCode::WaveAnyTrue;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.value = reader.GetMappedRelative(anchor);
            basicBlock->Append(instr);
            return true;
        }

        case DXILOpcodes::WaveAllTrue: {
            IL::WaveAllTrueInstruction instr{};
            instr.opCode = IL::OpCode::WaveAllTrue;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.value = reader.GetMappedRelative(anchor);
            basicBlock->Append(instr);
            return true;
        }

        case DXILOpcodes::WaveActiveBallot: {
            IL::WaveBallotInstruction instr{};
            instr.opCode = IL::OpCode::WaveBallot;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.value = reader.GetMappedRelative(anchor);
            basicBlock->Append(instr);
            return true;
        }

        case DXILOpcodes::WaveReadLaneAt: {
            IL::WaveReadInstruction instr{};
            instr.opCode = IL::OpCode::WaveRead;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.value = reader.GetMappedRelative(anchor);
            instr.lane = reader.GetMappedRelative(anchor);
            basicBlock->Append(instr);
            return true;
        }

        case DXILOpcodes::WaveActiveAllEqual: {
            IL::WaveAllEqualInstruction instr{};
            instr.opCode = IL::OpCode::WaveAllEqual;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.value = reader.GetMappedRelative(anchor);
            basicBlock->Append(instr);
            return true;
        }

        case DXILOpcodes::WaveActiveBit: {
            IL::ID value = reader.GetMappedRelative(anchor);

            uint64_t op = program.GetConstants().GetConstant<IL::IntConstant>(reader.GetMappedRelative(anchor))->value;
            switch (op) {
                default: {
                    ASSERT(false, "Unexpected op");
                    break;
                }
                case 0: {
                    IL::WaveBitAndInstruction instr{};
                    instr.result = result;
                    instr.source = IL::Source::Code(recordIdx);
                    instr.value = value;
                    instr.opCode = IL::OpCode::WaveBitAnd;
                    basicBlock->Append(instr);
                    break;
                }
                case 1: {
                    IL::WaveBitOrInstruction instr{};
                    instr.result = result;
                    instr.source = IL::Source::Code(recordIdx);
                    instr.value = value;
                    instr.opCode = IL::OpCode::WaveBitOr;
                    basicBlock->Append(instr);
                    break;
                }
                case 2: {
                    IL::WaveBitXOrInstruction instr{};
                    instr.result = result;
                    instr.source = IL::Source::Code(recordIdx);
                    instr.value = value;
                    instr.opCode = IL::OpCode::WaveBitXOr;
                    basicBlock->Append(instr);
                    break;
                }
            }
            return true;
        }

        case DXILOpcodes::WaveAllBitCount: {
            IL::WaveCountBitsInstruction instr{};
            instr.opCode = IL::OpCode::WaveCountBits;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.value = reader.GetMappedRelative(anchor);
            basicBlock->Append(instr);
            return true;
        }

        case DXILOpcodes::WaveActiveOp: {
            IL::ID value = reader.GetMappedRelative(anchor);

            uint64_t op = program.GetConstants().GetConstant<IL::IntConstant>(reader.GetMappedRelative(anchor))->value;
            switch (op) {
                default: {
                    ASSERT(false, "Unexpected op");
                    break;
                }
                case 0: {
                    IL::WaveSumInstruction instr{};
                    instr.result = result;
                    instr.source = IL::Source::Code(recordIdx);
                    instr.value = value;
                    instr.opCode = IL::OpCode::WaveSum;
                    basicBlock->Append(instr);
                    break;
                }
                case 1: {
                    IL::WaveProductInstruction instr{};
                    instr.result = result;
                    instr.source = IL::Source::Code(recordIdx);
                    instr.value = value;
                    instr.opCode = IL::OpCode::WaveProduct;
                    basicBlock->Append(instr);
                    break;
                }
                case 2: {
                    IL::WaveMinInstruction instr{};
                    instr.result = result;
                    instr.source = IL::Source::Code(recordIdx);
                    instr.value = value;
                    instr.opCode = IL::OpCode::WaveMin;
                    basicBlock->Append(instr);
                    break;
                }
                case 3: {
                    IL::WaveMaxInstruction instr{};
                    instr.result = result;
                    instr.source = IL::Source::Code(recordIdx);
                    instr.value = value;
                    instr.opCode = IL::OpCode::WaveMax;
                    basicBlock->Append(instr);
                    break;
                }
            }
            return true;
        }

        case DXILOpcodes::WavePrefixBitCount: {
            IL::WavePrefixCountBitsInstruction instr{};
            instr.opCode = IL::OpCode::WavePrefixCountBits;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.value = reader.GetMappedRelative(anchor);
            basicBlock->Append(instr);
            return true;
        }

        case DXILOpcodes::WavePrefixOp: {
            IL::ID value = reader.GetMappedRelative(anchor);

            uint64_t op = program.GetConstants().GetConstant<IL::IntConstant>(reader.GetMappedRelative(anchor))->value;
            switch (op) {
                default: {
                    ASSERT(false, "Unexpected op");
                    break;
                }
                case 0: {
                    IL::WavePrefixSumInstruction instr{};
                    instr.result = result;
                    instr.source = IL::Source::Code(recordIdx);
                    instr.value = value;
                    instr.opCode = IL::OpCode::WavePrefixSum;
                    basicBlock->Append(instr);
                    break;
                }
                case 1: {
                    IL::WavePrefixProductInstruction instr{};
                    instr.result = result;
                    instr.source = IL::Source::Code(recordIdx);
                    instr.value = value;
                    instr.opCode = IL::OpCode::WavePrefixProduct;
                    basicBlock->Append(instr);
                    break;
                }
            }
            return true;
        }

        /** SM6.6 annotation */
        case DXILOpcodes::AnnotateHandle: {
            // Get operands, ignore offset for now
            uint32_t resource = reader.GetMappedRelative(anchor);
            uint32_t properties = reader.GetMappedRelative(anchor);

            // Unused
            GRS_SINK(resource);

            // Get the annotation constant
            auto constant = program.GetConstants().GetConstant(properties);
            
            // Optional annotation
            DXILResourceProperties annotation{};

            // To property map
            switch (constant->kind) {
                default: {
                    break;
                }
                case Backend::IL::ConstantKind::Struct: {
                    auto _struct = constant->As<IL::StructConstant>();
                    annotation.basic.opaque = static_cast<uint32_t>(_struct->members.at(0)->As<IL::IntConstant>()->value);
                    annotation.typed.opaque = static_cast<uint32_t>(_struct->members.at(1)->As<IL::IntConstant>()->value);
                    break;
                }
                case Backend::IL::ConstantKind::Null: {
                    // Null constants are allowed
                    break;
                }
            }

            // Set from annotation
            ilTypeMap.SetType(result, GetTypeFromProperties(annotation));
            
            // Emit as unexposed
            IL::UnexposedInstruction instr{};
            instr.opCode = IL::OpCode::Unexposed;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.backendOpCode = 0x0;
            instr.symbol = "AnnotateHandle";
            basicBlock->Append(instr);
            return true;
        }

        case DXILOpcodes::CreateHandleFromHeap: {
            // Get operands, ignore offset for now
            uint32_t index = reader.GetMappedRelative(anchor);
            uint32_t sampler = reader.GetMappedRelative(anchor);
            uint32_t nonUniform = reader.GetMappedRelative(anchor);

            // Unused
            GRS_SINK(index, sampler, nonUniform);
            
            // Emit as unexposed
            IL::UnexposedInstruction instr{};
            instr.opCode = IL::OpCode::Unexposed;
            instr.result = result;
            instr.source = IL::Source::Code(recordIdx);
            instr.backendOpCode = 0x0;
            instr.symbol = "CreateHandleFromHeap";
            basicBlock->Append(instr);
            return true;
        }
    }
}

const Backend::IL::Type* DXILPhysicalBlockFunction::GetTypeFromProperties(const DXILResourceProperties& properties) {
    Backend::IL::TypeMap &types = program.GetTypeMap();

    // Handle on shape
    switch (static_cast<DXILShaderResourceShape>(properties.basic.shape)) {
        default: {
            return types.FindTypeOrAdd(Backend::IL::UnexposedType{});
        }
        case DXILShaderResourceShape::Texture1D:
        case DXILShaderResourceShape::Texture2D:
        case DXILShaderResourceShape::Texture2DMS:
        case DXILShaderResourceShape::Texture3D:
        case DXILShaderResourceShape::TextureCube:
        case DXILShaderResourceShape::Texture1DArray:
        case DXILShaderResourceShape::Texture2DArray:
        case DXILShaderResourceShape::Texture2DMSArray:
        case DXILShaderResourceShape::TextureCubeArray: {
            return GetTypeFromTextureProperties(properties);
        }
        case DXILShaderResourceShape::TypedBuffer:
        case DXILShaderResourceShape::RawBuffer:
        case DXILShaderResourceShape::StructuredBuffer: {
            return GetTypeFromBufferProperties(properties);
        }
        case DXILShaderResourceShape::CBuffer: {
            return types.FindTypeOrAdd(Backend::IL::CBufferType{});
        }
        case DXILShaderResourceShape::Sampler: {
            return types.FindTypeOrAdd(Backend::IL::SamplerType{});
        }
    }
}

const Backend::IL::Type* DXILPhysicalBlockFunction::GetTypeFromTextureProperties(const DXILResourceProperties& properties) {
    Backend::IL::TypeMap &types = program.GetTypeMap();

    // Final format
    Backend::IL::Format format = table.metadata.GetComponentFormat(static_cast<ComponentType>(properties.typed.resource.componentType));

    // Final sampled type
    const Backend::IL::Type* sampledType = table.metadata.GetComponentType(static_cast<ComponentType>(properties.typed.resource.componentType));

    // Vectorized?
    if (properties.typed.resource.componentCount > 1) {
        sampledType = types.FindTypeOrAdd(Backend::IL::VectorType {
            .containedType = sampledType,
            .dimension = properties.typed.resource.componentCount
        });
    }

    // Translate dimension
    Backend::IL::TextureDimension dimension;
    switch (static_cast<DXILShaderResourceShape>(properties.basic.shape)) {
        default: {
            ASSERT(false, "Unexpected shape");
            return nullptr;
        }
        case DXILShaderResourceShape::Texture1D:
            dimension = Backend::IL::TextureDimension::Texture1D;
            break;
        case DXILShaderResourceShape::Texture2D:
        case DXILShaderResourceShape::Texture2DMS:
            dimension = Backend::IL::TextureDimension::Texture2D;
            break;
        case DXILShaderResourceShape::Texture3D:
            dimension = Backend::IL::TextureDimension::Texture3D;
            break;
        case DXILShaderResourceShape::TextureCube:
            dimension = Backend::IL::TextureDimension::Texture2DCube;
            break;
        case DXILShaderResourceShape::Texture1DArray:
            dimension = Backend::IL::TextureDimension::Texture1DArray;
            break;
        case DXILShaderResourceShape::Texture2DArray:
            dimension = Backend::IL::TextureDimension::Texture2DArray;
            break;
        case DXILShaderResourceShape::Texture2DMSArray:
            dimension = Backend::IL::TextureDimension::Texture2DArray;
            break;
        case DXILShaderResourceShape::TextureCubeArray:
            dimension = Backend::IL::TextureDimension::Texture2DCubeArray;
            break;
    }

    // Create type
    return types.FindTypeOrAdd(Backend::IL::TextureType{
        .sampledType = sampledType,
        .dimension = dimension,
        .multisampled = properties.typed.resource.sampleCount > 1,
        .samplerMode = properties.basic.isUAV ? Backend::IL::ResourceSamplerMode::Writable : Backend::IL::ResourceSamplerMode::RuntimeOnly,
        .format = format
    });
}

const Backend::IL::Type * DXILPhysicalBlockFunction::GetTypeFromBufferProperties(const DXILResourceProperties &properties) {
    Backend::IL::TypeMap &types = program.GetTypeMap();

    // Final format
    Backend::IL::Format format = table.metadata.GetComponentFormat(static_cast<ComponentType>(properties.typed.resource.componentType));

    // Final sampled type
    const Backend::IL::Type* sampledType = table.metadata.GetComponentType(static_cast<ComponentType>(properties.typed.resource.componentType));

    // Vectorized?
    if (properties.typed.resource.componentCount > 1) {
        sampledType = types.FindTypeOrAdd(Backend::IL::VectorType {
            .containedType = sampledType,
            .dimension = properties.typed.resource.componentCount
        });
    }

    // Create type
    return types.FindTypeOrAdd(Backend::IL::BufferType{
        .elementType = sampledType,
        .samplerMode = properties.basic.isUAV ? Backend::IL::ResourceSamplerMode::Writable : Backend::IL::ResourceSamplerMode::RuntimeOnly,
        .texelType = format
    });
}

bool DXILPhysicalBlockFunction::IsSVOX(IL::ID value) {
    switch (table.idRemapper.GetUserMappingType(value)) {
        default:
            return false;
        case DXILIDUserType::VectorOnStruct:
        case DXILIDUserType::VectorOnSequential:
        case DXILIDUserType::StructOnSequential:
            return true;
    }
}

uint32_t DXILPhysicalBlockFunction::GetSVOXCount(IL::ID value) {
    // Get type
    const auto* lhsType = program.GetTypeMap().GetType(value);

    // Determine count
    switch (table.idRemapper.GetUserMappingType(value)) {
        default:
            ASSERT(false, "Invalid id type");
            return ~0u;
        case DXILIDUserType::Singular: {
            return 1;
        }
        case DXILIDUserType::VectorOnStruct:
        case DXILIDUserType::StructOnSequential: {
            const auto* _struct = lhsType->As<Backend::IL::StructType>();
            return static_cast<uint32_t>(_struct->memberTypes.size());
        }
        case DXILIDUserType::VectorOnSequential: {
            const auto* vector = lhsType->As<Backend::IL::VectorType>();
            return vector->dimension;
        }
    }
}

IL::ID DXILPhysicalBlockFunction::AllocateSVOSequential(uint32_t count, IL::ID x, IL::ID y, IL::ID z, IL::ID w) {
    // Pass through if singular
    if (count == 1) {
        return x;
    }

    // Get type, all share the same type in effect
    const Backend::IL::Type* type = program.GetTypeMap().GetType(x);

    // Emulated value
    IL::ID svox = program.GetIdentifierMap().AllocID();

    // Vectorize coordinate
    IL::ID base = program.GetIdentifierMap().AllocIDRange(count);
    if (count > 0)
        table.idRemapper.AllocSourceUserMapping(base + 0, DXILIDUserType::Singular, x);
    if (count > 1)
        table.idRemapper.AllocSourceUserMapping(base + 1, DXILIDUserType::Singular, y);
    if (count > 2)
        table.idRemapper.AllocSourceUserMapping(base + 2, DXILIDUserType::Singular, z);
    if (count > 3)
        table.idRemapper.AllocSourceUserMapping(base + 3, DXILIDUserType::Singular, w);

    // Set base
    table.idRemapper.AllocSourceUserMapping(svox, DXILIDUserType::VectorOnSequential, base);

    // Set type
    program.GetTypeMap().SetType(svox, program.GetTypeMap().FindTypeOrAdd(Backend::IL::VectorType{
        .containedType = type,
        .dimension = static_cast<uint8_t>(count)
    }));

    // OK
    return svox;
}

IL::ID DXILPhysicalBlockFunction::AllocateSVOStructSequential(const Backend::IL::Type* type, const IL::ID *values, uint32_t count) {
    // Pass through if singular
    if (count == 1) {
        return values[0];
    }

    // Emulated value
    IL::ID svox = program.GetIdentifierMap().AllocID();

    // Fill out range
    IL::ID base = program.GetIdentifierMap().AllocIDRange(count);
    for (uint32_t i = 0; i < count; i++) {
        table.idRemapper.AllocSourceUserMapping(base + i, DXILIDUserType::Singular, values[i]);
    }

    // Set base
    table.idRemapper.AllocSourceUserMapping(svox, DXILIDUserType::StructOnSequential, base);

    // Set type
    program.GetTypeMap().SetType(svox, type);

    // OK
    return svox;
}

DXILPhysicalBlockFunction::SVOXElement DXILPhysicalBlockFunction::ExtractSVOXElement(LLVMBlock* block, IL::ID value, uint32_t index) {
    // Get type
    const auto* lhsType = program.GetTypeMap().GetType(value);

    // Determine count
    switch (table.idRemapper.GetUserMappingType(value)) {
        default: {
            ASSERT(false, "Invalid id type");
            return {nullptr, IL::InvalidID};
        }
        case DXILIDUserType::Singular: {
            return {lhsType, value};
        }
        case DXILIDUserType::VectorOnStruct: {
            const auto* vector = lhsType->As<Backend::IL::VectorType>();

            IL::ID extractedId = program.GetIdentifierMap().AllocID();

            // Extract current value
            LLVMRecord recordExtract(LLVMFunctionRecord::InstExtractVal);
            recordExtract.SetUser(true, ~0u, extractedId);
            recordExtract.opCount = 2;
            recordExtract.ops = table.recordAllocator.AllocateArray<uint64_t>(2);
            recordExtract.ops[0] = DXILIDRemapper::EncodeUserOperand(value);
            recordExtract.ops[1] = index;
            block->AddRecord(recordExtract);

            // Invoke on extracted value
            return {vector->containedType, extractedId};
        }
        case DXILIDUserType::VectorOnSequential: {
            const auto* vector = lhsType->As<Backend::IL::VectorType>();
            uint32_t base = table.idRemapper.TryGetUserMapping(value);
            return {vector->containedType, table.idRemapper.TryGetUserMapping(base + index)};
        }
        case DXILIDUserType::StructOnSequential: {
            const auto* _struct = lhsType->As<Backend::IL::StructType>();
            uint32_t base = table.idRemapper.TryGetUserMapping(value);
            return {_struct->memberTypes[index], table.idRemapper.TryGetUserMapping(base + index)};
        }
    }
}

template<typename F>
void DXILPhysicalBlockFunction::IterateSVOX(LLVMBlock* block, IL::ID value, F&& functor) {
    DXILIDUserType idType = table.idRemapper.GetUserMappingType(value);

    // Get type
    const auto* type = program.GetTypeMap().GetType(value);

    // Pass through if singular
    if (idType == DXILIDUserType::Singular) {
        functor(type, value, 0u, 1u);
        return;
    }

    // Get component count
    uint32_t count = GetSVOXCount(value);

    // Visit all cases
    for (uint32_t i = 0; i < count; i++) {
        SVOXElement element = ExtractSVOXElement(block, value, i);
        functor(element.type, element.value, i, count);
    }
}

template<typename F>
void DXILPhysicalBlockFunction::UnaryOpSVOX(LLVMBlock *block, IL::ID result, IL::ID value, F &&functor) {
    DXILIDUserType idType = table.idRemapper.GetUserMappingType(value);

    // Get type
    const auto* type = program.GetTypeMap().GetType(value);

    // Pass through if singular
    if (idType == DXILIDUserType::Singular) {
        functor(type, result, value);
        table.idRemapper.AllocSourceUserMapping(result, DXILIDUserType::Singular, result);
        return;
    }

    // Get component count
    uint32_t count = GetSVOXCount(value);

    // Allocate base index
    IL::ID base = program.GetIdentifierMap().AllocIDRange(count);

    // Visit all cases
    for (uint32_t i = 0; i < count; i++) {
        SVOXElement element = ExtractSVOXElement(block, value, i);

        // Allocate component result
        IL::ID componentResult = program.GetIdentifierMap().AllocID();

        // Allocate result as sequential
        table.idRemapper.AllocSourceUserMapping(base + i, DXILIDUserType::Singular, componentResult);

        // Invoke functor
        functor(element.type, componentResult, element.value);
    }

    // Mark final result as VOS
    table.idRemapper.AllocSourceUserMapping(result, DXILIDUserType::VectorOnSequential, base);
}

template<typename F>
void DXILPhysicalBlockFunction::BinaryOpSVOX(LLVMBlock* block, IL::ID result, IL::ID lhs, IL::ID rhs, F&& functor) {
    DXILIDUserType lhsIdType = table.idRemapper.GetUserMappingType(lhs);
    DXILIDUserType rhsIdType = table.idRemapper.GetUserMappingType(rhs);

    // Get types
    const auto* lhsType = program.GetTypeMap().GetType(lhs);

    // Singular operations are pass through
    if (lhsIdType == DXILIDUserType::Singular) {
        ASSERT(rhsIdType == DXILIDUserType::Singular, "Singular operations must match");
        return functor(lhsType, result, lhs, rhs);
    }

    // Get component count
    uint32_t count = GetSVOXCount(lhs);

    // Allocate base index
    IL::ID base = program.GetIdentifierMap().AllocIDRange(count);

    // Handle all cases
    for (uint32_t i = 0; i < count; i++) {
        SVOXElement lhsElement = ExtractSVOXElement(block, lhs, i);
        SVOXElement rhsElement = ExtractSVOXElement(block, rhs, i);

        // Allocate component result
        IL::ID componentResult = program.GetIdentifierMap().AllocID();

        // Allocate result as sequential
        table.idRemapper.AllocSourceUserMapping(base + i, DXILIDUserType::Singular, componentResult);

        // Invoke functor
        functor(lhsElement.type, componentResult, lhsElement.value, rhsElement.value);
    }

    // Mark final result as VOS
    table.idRemapper.AllocSourceUserMapping(result, DXILIDUserType::VectorOnSequential, base);
}

static bool IsFunctionPostRecordDependentBlock(LLVMReservedBlock block) {
    switch (block) {
        default:
            return false;
        case LLVMReservedBlock::ValueSymTab:
        case LLVMReservedBlock::UseList:
        case LLVMReservedBlock::MetadataAttachment:
            return true;
    }
}

void DXILPhysicalBlockFunction::CompileFunction(const DXCompileJob& job, struct LLVMBlock *block) {
    const uint32_t functionIndex = static_cast<uint32_t>(functionBlocks.Size());

    // Definition order is linear to the function blocks
    uint32_t linkedIndex = internalLinkedFunctions[functionIndex];

    // Get function definition
    DXILFunctionDeclaration *declaration = functions[linkedIndex];

    // Branching handling for multi function setups
    if (RequiresValueMapSegmentation()) {
        // Merge the id value segment
        table.idMap.Merge(declaration->segments.idSegment);
    }

    // Create a new function block
    FunctionBlock& functionBlock = functionBlocks.Add();
    functionBlock.uid = block->uid;
    functionBlock.recordRelocation.Resize(block->records.size());

    // Default to no relocation
    std::fill_n(functionBlock.recordRelocation.Data(), functionBlock.recordRelocation.Size(), IL::InvalidID);

    // Get function
    IL::Function *fn = program.GetFunctionList()[functionIndex];

    // Remap all blocks by dominance
    if (!fn->ReorderByDominantBlocks(false)) {
        return;
    }

    // Visit child blocks
    for (LLVMBlock *fnBlock: block->blocks) {
        switch (static_cast<LLVMReservedBlock>(fnBlock->id)) {
            default: {
                break;
            }
            case LLVMReservedBlock::Constants: {
                table.global.CompileConstants(fnBlock);
                break;
            }
        }
    }

    // Get the program map
    Backend::IL::TypeMap &typeMap = program.GetTypeMap();

    // Swap source data
    Vector<LLVMRecord> source(allocators);
    block->records.swap(source);

    // Swap element data
    Vector<LLVMBlockElement> elements(allocators);
    block->elements.swap(elements);

    // Reserve
    block->elements.reserve(elements.size());

    // Filter all records
    for (const LLVMBlockElement &element: elements) {
        switch (static_cast<LLVMBlockElementType>(element.type)) {
            case LLVMBlockElementType::Record: {
                break;
            }
            case LLVMBlockElementType::Abbreviation: {
                block->elements.push_back(element);
                break;
            }
            case LLVMBlockElementType::Block: {
                if (!IsFunctionPostRecordDependentBlock(static_cast<LLVMReservedBlock>(block->blocks[element.id]->id))) {
                    block->elements.push_back(element);
                }
                break;
            }
        }
    }

    // Linear block to il mapper
    std::unordered_map<IL::ID, uint32_t> branchMappings;

    // Create mappings
    for (const IL::BasicBlock *bb: fn->GetBasicBlocks()) {
        branchMappings[bb->GetID()] = static_cast<uint32_t>(branchMappings.size());
    }

    // Emit the number of blocks
    LLVMRecord declareBlocks(LLVMFunctionRecord::DeclareBlocks);
    declareBlocks.opCount = 1;
    declareBlocks.ops = table.recordAllocator.AllocateArray<uint64_t>(1);
    declareBlocks.ops[0] = fn->GetBasicBlocks().GetBlockCount();
    block->InsertRecord(block->elements.data(), declareBlocks);

    // Add binding handles
    CreateHandles(job, block);

    // Compile all blocks
    for (const IL::BasicBlock *bb: fn->GetBasicBlocks()) {
        // Compile all instructions
        for (const IL::Instruction *instr: *bb) {
            LLVMRecord record;

            // If symbolic, nothing to do
            if (instr->source.symbolic) {
                continue;
            }

            // If it's valid, copy record
            if (instr->source.IsValid()) {
                // Set relocation index
                functionBlock.recordRelocation[instr->source.codeOffset - 1u] = static_cast<uint32_t>(block->records.size()) - 1u;

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

            // Setup writer
            DXILValueWriter writer(table, record);

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
                    table.idRemapper.SetUserRedirect(instr->result, constant->id);
                    break;
                }

                    /* Atomic binary ops */
                case IL::OpCode::AtomicOr:
                case IL::OpCode::AtomicXOr:
                case IL::OpCode::AtomicAnd:
                case IL::OpCode::AtomicAdd:
                case IL::OpCode::AtomicMin:
                case IL::OpCode::AtomicMax:
                case IL::OpCode::AtomicExchange:
                case IL::OpCode::AtomicCompareExchange: {
                    // Resulting binary operation
                    DXILAtomicBinOp binOp;

                    // Identifiers
                    IL::ID valueID;
                    IL::ID addressID;

                    // Get operands and determine appropriate op
                    switch (instr->opCode) {
                        default:
                            ASSERT(false, "Invalid opcode");
                            break;
                        case IL::OpCode::AtomicOr:
                            addressID = instr->As<IL::AtomicOrInstruction>()->address;
                            valueID = instr->As<IL::AtomicOrInstruction>()->value;
                            binOp = DXILAtomicBinOp::Or;
                            break;
                        case IL::OpCode::AtomicXOr:
                            addressID = instr->As<IL::AtomicXOrInstruction>()->address;
                            valueID = instr->As<IL::AtomicXOrInstruction>()->value;
                            binOp = DXILAtomicBinOp::XOr;
                            break;
                        case IL::OpCode::AtomicAnd:
                            addressID = instr->As<IL::AtomicAndInstruction>()->address;
                            valueID = instr->As<IL::AtomicAndInstruction>()->value;
                            binOp = DXILAtomicBinOp::And;
                            break;
                        case IL::OpCode::AtomicAdd:
                            addressID = instr->As<IL::AtomicAddInstruction>()->address;
                            valueID = instr->As<IL::AtomicAddInstruction>()->value;
                            binOp = DXILAtomicBinOp::Add;
                            break;
                        case IL::OpCode::AtomicMin: {
                            auto* _instr = instr->As<IL::AtomicMinInstruction>();
                            addressID = _instr->address;
                            valueID = _instr->value;

                            const Backend::IL::Type* valueType = program.GetTypeMap().GetType(_instr->value);
                            ASSERT(valueType->Is<Backend::IL::IntType>(), "Atomic operation on non-integer type");

                            binOp = valueType->As<Backend::IL::IntType>()->signedness ? DXILAtomicBinOp::IMin : DXILAtomicBinOp::UMin;
                            break;
                        }
                        case IL::OpCode::AtomicMax: {
                            auto* _instr = instr->As<IL::AtomicMaxInstruction>();
                            addressID = _instr->address;
                            valueID = _instr->value;

                            const Backend::IL::Type* valueType = program.GetTypeMap().GetType(_instr->value);
                            ASSERT(valueType->Is<Backend::IL::IntType>(), "Atomic operation on non-integer type");

                            binOp = valueType->As<Backend::IL::IntType>()->signedness ? DXILAtomicBinOp::IMax : DXILAtomicBinOp::UMax;
                            break;
                        }
                        case IL::OpCode::AtomicExchange:
                            addressID = instr->As<IL::AtomicExchangeInstruction>()->address;
                            valueID = instr->As<IL::AtomicExchangeInstruction>()->value;
                            binOp = DXILAtomicBinOp::Exchange;
                            break;
                        case IL::OpCode::AtomicCompareExchange:
                            addressID = instr->As<IL::AtomicCompareExchangeInstruction>()->address;
                            valueID = instr->As<IL::AtomicCompareExchangeInstruction>()->value;
                            binOp = DXILAtomicBinOp::Invalid;
                            break;
                    }

                    // Get source address
                    IL::InstructionRef<> addressInstr = program.GetIdentifierMap().Get(addressID);

                    // Different atomic intrinsics depending on the source
                    switch (addressInstr->opCode) {
                        default:
                            ASSERT(false, "Unsupported atomic operation");
                            break;
                        case IL::OpCode::AddressChain: {
                            auto* chainInstr = addressInstr->As<Backend::IL::AddressChainInstruction>();
                            ASSERT(chainInstr->chains.count == 1, "Multi-chain on atomic operations not supported");

                            // Get handle of address base
                            const auto* addressType = program.GetTypeMap().GetType(chainInstr->result)->As<Backend::IL::PointerType>();;

                            // Handle based atomic?
                            if (addressType->addressSpace == Backend::IL::AddressSpace::Texture || addressType->addressSpace == Backend::IL::AddressSpace::Buffer) {
                                if (instr->opCode == Backend::IL::OpCode::AtomicCompareExchange) {
                                    auto _instr = instr->As<Backend::IL::AtomicCompareExchangeInstruction>();

                                    // Get intrinsic
                                    const DXILFunctionDeclaration *intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpAtomicCompareExchangeI32);

                                    /*
                                     * ; overloads: SM5.1: i32,  SM6.0: i32
                                     * ; returns: original value in memory before the operation
                                     * declare i32 @dx.op.atomicCompareExchange.i32(
                                     *     i32,                  ; opcode
                                     *     %dx.types.Handle,     ; resource handle
                                     *     i32,                  ; coordinate c0
                                     *     i32,                  ; coordinate c1
                                     *     i32,                  ; coordinate c2
                                     *     i32,                  ; comparison value
                                     *     i32)                  ; new value
                                     */

                                    uint64_t ops[7];

                                    ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                                        Backend::IL::IntConstant{.value = static_cast<uint32_t>(DXILOpcodes::AtomicCompareExchange)}
                                    )->id);

                                    ops[1] = table.idRemapper.EncodeRedirectedUserOperand(chainInstr->composite);

                                    ops[2] = table.idRemapper.EncodeRedirectedUserOperand(chainInstr->chains[0].index);

                                    ops[3] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                                        Backend::IL::UndefConstant{}
                                    )->id);

                                    ops[4] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                                        Backend::IL::UndefConstant{}
                                    )->id);

                                    ops[5] = table.idRemapper.EncodeRedirectedUserOperand(_instr->comparator);

                                    ops[6] = table.idRemapper.EncodeRedirectedUserOperand(_instr->value);

                                    // Invoke
                                    block->AddRecord(CompileIntrinsicCall(instr->result, intrinsic, 7, ops));
                                } else {
                                    // Get intrinsic
                                    const DXILFunctionDeclaration *intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpAtomicBinOpI32);

                                    /*
                                     * ; overloads: SM5.1: i32,  SM6.0: i32
                                     * ; returns: original value in memory before the operation
                                     * declare i32 @dx.op.atomicBinOp.i32(
                                     *     i32,                  ; opcode
                                     *     %dx.types.Handle,     ; resource handle
                                     *     i32,                  ; binary operation code: EXCHANGE, IADD, AND, OR, XOR, IMIN, IMAX, UMIN, UMAX
                                     *     i32,                  ; coordinate c0
                                     *     i32,                  ; coordinate c1
                                     *     i32,                  ; coordinate c2
                                     *     i32)                  ; new value
                                     */

                                    uint64_t ops[7];

                                    ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                                        Backend::IL::IntConstant{.value = static_cast<uint32_t>(DXILOpcodes::AtomicBinOp)}
                                    )->id);

                                    ops[1] = table.idRemapper.EncodeRedirectedUserOperand(chainInstr->composite);

                                    ops[2] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                                        Backend::IL::IntConstant{.value = static_cast<uint32_t>(binOp)}
                                    )->id);

                                    ops[3] = table.idRemapper.EncodeRedirectedUserOperand(chainInstr->chains[0].index);

                                    ops[4] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                                        Backend::IL::UndefConstant{}
                                    )->id);

                                    ops[5] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                                        Backend::IL::UndefConstant{}
                                    )->id);

                                    ops[6] = table.idRemapper.EncodeRedirectedUserOperand(valueID);

                                    // Invoke
                                    block->AddRecord(CompileIntrinsicCall(instr->result, intrinsic, 7, ops));
                                }
                            } else {
                                ASSERT(false, "Non-handle atomic compilation not supported");
                            }
                            break;
                        }
                    }

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

                    // Translate op code
                    LLVMBinOp opCode{};
                    switch (instr->opCode) {
                        default:
                        ASSERT(false, "Unexpected opcode in instruction");
                            break;
                        case IL::OpCode::Add: {
                            auto _instr = instr->As<IL::AddInstruction>();

                            // Handle as binary
                            BinaryOpSVOX(block, instr->result, _instr->lhs, _instr->rhs, [&](const Backend::IL::Type* type, IL::ID result, IL::ID lhs, IL::ID rhs) {
                                record.SetUser(true, ~0u, result);
                                record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                                record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(lhs);
                                record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(rhs);
                                opCode = LLVMBinOp::Add;

                                // Set bin op
                                record.ops[2] = static_cast<uint64_t>(opCode);
                                block->AddRecord(record);
                            });
                            break;
                        }
                        case IL::OpCode::Sub: {
                            auto _instr = instr->As<IL::SubInstruction>();

                            // Handle as binary
                            BinaryOpSVOX(block, instr->result, _instr->lhs, _instr->rhs, [&](const Backend::IL::Type* type, IL::ID result, IL::ID lhs, IL::ID rhs) {
                                record.SetUser(true, ~0u, result);
                                record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                                record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(lhs);
                                record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(rhs);
                                record.ops[2] = static_cast<uint64_t>(LLVMBinOp::Sub);
                                block->AddRecord(record);
                            });
                            break;
                        }
                        case IL::OpCode::Div: {
                            auto _instr = instr->As<IL::DivInstruction>();

                            // Handle as binary
                            BinaryOpSVOX(block, instr->result, _instr->lhs, _instr->rhs, [&](const Backend::IL::Type* type, IL::ID result, IL::ID lhs, IL::ID rhs) {
                                record.SetUser(true, ~0u, result);
                                record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                                record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(lhs);
                                record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(rhs);

                                if (type->Is<Backend::IL::FPType>()) {
                                    opCode = LLVMBinOp::SDiv;
                                } else if (auto intType = type->Cast<Backend::IL::IntType>()) {
                                    opCode = intType->signedness ? LLVMBinOp::SDiv : LLVMBinOp::UDiv;
                                } else {
                                    ASSERT(false, "Invalid type in Div");
                                }

                                record.ops[2] = static_cast<uint64_t>(opCode);
                                block->AddRecord(record);
                            });
                            break;
                        }
                        case IL::OpCode::Mul: {
                            auto _instr = instr->As<IL::MulInstruction>();

                            // Handle as binary
                            BinaryOpSVOX(block, instr->result, _instr->lhs, _instr->rhs, [&](const Backend::IL::Type* type, IL::ID result, IL::ID lhs, IL::ID rhs) {
                                record.SetUser(true, ~0u, result);
                                record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                                record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(lhs);
                                record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(rhs);
                                record.ops[2] = static_cast<uint64_t>(LLVMBinOp::Mul);
                                block->AddRecord(record);
                            });
                            break;
                        }
                        case IL::OpCode::Or: {
                            auto _instr = instr->As<IL::OrInstruction>();

                            // Handle as binary
                            BinaryOpSVOX(block, instr->result, _instr->lhs, _instr->rhs, [&](const Backend::IL::Type* type, IL::ID result, IL::ID lhs, IL::ID rhs) {
                                record.SetUser(true, ~0u, result);
                                record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                                record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(lhs);
                                record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(rhs);
                                record.ops[2] = static_cast<uint64_t>(LLVMBinOp::Or);
                                block->AddRecord(record);
                            });
                            break;
                        }
                        case IL::OpCode::BitOr: {
                            auto _instr = instr->As<IL::BitOrInstruction>();

                            // Handle as binary
                            BinaryOpSVOX(block, instr->result, _instr->lhs, _instr->rhs, [&](const Backend::IL::Type* type, IL::ID result, IL::ID lhs, IL::ID rhs) {
                                record.SetUser(true, ~0u, result);
                                record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                                record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(lhs);
                                record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(rhs);
                                record.ops[2] = static_cast<uint64_t>(LLVMBinOp::Or);
                                block->AddRecord(record);
                            });
                            break;
                        }
                        case IL::OpCode::BitXOr: {
                            auto _instr = instr->As<IL::BitXOrInstruction>();

                            // Handle as binary
                            BinaryOpSVOX(block, instr->result, _instr->lhs, _instr->rhs, [&](const Backend::IL::Type* type, IL::ID result, IL::ID lhs, IL::ID rhs) {
                                record.SetUser(true, ~0u, result);
                                record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                                record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(lhs);
                                record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(rhs);
                                record.ops[2] = static_cast<uint64_t>(LLVMBinOp::XOr);
                                block->AddRecord(record);
                            });
                            break;
                        }
                        case IL::OpCode::And: {
                            auto _instr = instr->As<IL::AndInstruction>();

                            // Handle as binary
                            BinaryOpSVOX(block, instr->result, _instr->lhs, _instr->rhs, [&](const Backend::IL::Type* type, IL::ID result, IL::ID lhs, IL::ID rhs) {
                                record.SetUser(true, ~0u, result);
                                record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                                record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(lhs);
                                record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(rhs);
                                record.ops[2] = static_cast<uint64_t>(LLVMBinOp::And);
                                block->AddRecord(record);
                            });
                            break;
                        }
                        case IL::OpCode::BitAnd: {
                            auto _instr = instr->As<IL::BitAndInstruction>();

                            // Handle as binary
                            BinaryOpSVOX(block, instr->result, _instr->lhs, _instr->rhs, [&](const Backend::IL::Type* type, IL::ID result, IL::ID lhs, IL::ID rhs) {
                                record.SetUser(true, ~0u, result);
                                record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                                record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(lhs);
                                record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(rhs);
                                record.ops[2] = static_cast<uint64_t>(LLVMBinOp::And);
                                block->AddRecord(record);
                            });
                            break;
                        }
                        case IL::OpCode::BitShiftLeft: {
                            auto _instr = instr->As<IL::BitShiftLeftInstruction>();

                            // Handle as binary
                            BinaryOpSVOX(block, instr->result, _instr->value, _instr->shift, [&](const Backend::IL::Type* type, IL::ID result, IL::ID value, IL::ID shift) {
                                record.SetUser(true, ~0u, result);
                                record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                                record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(value);
                                record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(shift);
                                record.ops[2] = static_cast<uint64_t>(LLVMBinOp::SHL);
                                block->AddRecord(record);
                            });
                            break;
                        }
                        case IL::OpCode::BitShiftRight: {
                            auto _instr = instr->As<IL::BitShiftRightInstruction>();

                            // Handle as binary
                            BinaryOpSVOX(block, instr->result, _instr->value, _instr->shift, [&](const Backend::IL::Type* type, IL::ID result, IL::ID value, IL::ID shift) {
                                record.SetUser(true, ~0u, result);
                                record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                                record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(value);
                                record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(shift);
                                record.ops[2] = static_cast<uint64_t>(LLVMBinOp::AShR);
                                block->AddRecord(record);
                            });
                            break;
                        }
                        case IL::OpCode::Rem: {
                            auto _instr = instr->As<IL::RemInstruction>();

                            // Handle as binary
                            BinaryOpSVOX(block, instr->result, _instr->lhs, _instr->rhs, [&](const Backend::IL::Type* type, IL::ID result, IL::ID lhs, IL::ID rhs) {
                                record.SetUser(true, ~0u, result);
                                record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                                record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(lhs);
                                record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(rhs);

                                if (type->Is<Backend::IL::FPType>()) {
                                    opCode = LLVMBinOp::SRem;
                                } else if (auto intType = type->Cast<Backend::IL::IntType>()) {
                                    opCode = intType->signedness ? LLVMBinOp::SRem : LLVMBinOp::URem;
                                } else {
                                    ASSERT(false, "Invalid type in Rem");
                                }
                                record.ops[2] = static_cast<uint64_t>(opCode);
                                block->AddRecord(record);
                            });
                            break;
                        }
                    }
                    break;
                }

                case IL::OpCode::Not: {
                    auto _instr = instr->As<IL::NotInstruction>();

                    // Prepare record
                    record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstBinOp);
                    record.opCount = 3;
                    
                    // Handle as unary
                    UnaryOpSVOX(block, instr->result, _instr->value, [&](const Backend::IL::Type* type, IL::ID result, IL::ID value) {
                        record.SetUser(true, ~0u, result);
                        record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                        record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(value);
                        record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                            program.GetTypeMap().FindTypeOrAdd(Backend::IL::BoolType{}),
                            Backend::IL::BoolConstant{.value = true}
                        )->id);
                        record.ops[2] = static_cast<uint64_t>(LLVMBinOp::XOr);
                        block->AddRecord(record);
                    });
                    break;
                }

                case IL::OpCode::WaveAllEqual: {
                    auto _instr = instr->As<IL::WaveAllEqualInstruction>();
                    table.metadata.AddProgramFlag(DXILProgramShaderFlag::UseWaveIntrinsics);

                    // Get intrinsic
                    const DXILFunctionDeclaration *intrinsic;
                    switch (typeMap.GetType(_instr->value)->kind) {
                        default:
                            ASSERT(false, "Invalid bit width");
                            return;
                        case Backend::IL::TypeKind::Int:
                            intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpWaveActiveAllEqualI32);
                            break;
                        case Backend::IL::TypeKind::FP:
                            break;
                    }
                    
                    
                    uint64_t ops[2];
                    ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(static_cast<uint32_t>(DXILOpcodes::WaveActiveAllEqual))->id);
                    ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->value);
                    block->AddRecord(CompileIntrinsicCall(_instr->result, intrinsic, 2, ops));
                    break;
                }
                
                case IL::OpCode::ResourceSize: {
                    auto _instr = instr->As<IL::ResourceSizeInstruction>();

                    // Get intrinsic
                    const DXILFunctionDeclaration *intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpGetDimensions);

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

                        // Set as VOS
                        table.idRemapper.AllocSourceUserMapping(_instr->result, DXILIDUserType::VectorOnStruct, 0);
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

                            // Set cmp op
                            record.ops[2] = static_cast<uint64_t>(opCode);
                            block->AddRecord(record);
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

                            // Set cmp op
                            record.ops[2] = static_cast<uint64_t>(opCode);
                            block->AddRecord(record);
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

                            // Set cmp op
                            record.ops[2] = static_cast<uint64_t>(opCode);
                            block->AddRecord(record);
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

                            // Set cmp op
                            record.ops[2] = static_cast<uint64_t>(opCode);
                            block->AddRecord(record);
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

                            // Set cmp op
                            record.ops[2] = static_cast<uint64_t>(opCode);
                            block->AddRecord(record);
                            break;
                        }
                        case IL::OpCode::GreaterThanEqual: {
                            auto _instr = instr->As<IL::GreaterThanEqualInstruction>();

                            BinaryOpSVOX(block, _instr->result, _instr->lhs, _instr->rhs, [&](const Backend::IL::Type* type, IL::ID result, IL::ID lhs, IL::ID rhs) {
                                record.SetUser(true, ~0u, result);
                                record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                                record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(lhs);
                                record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(rhs);

                                if (type->Is<Backend::IL::FPType>()) {
                                    opCode = LLVMCmpOp::FloatUnorderedGreaterEqual;
                                } else if (auto intType = type->Cast<Backend::IL::IntType>()) {
                                    opCode = intType->signedness ? LLVMCmpOp::IntSignedGreaterEqual : LLVMCmpOp::IntUnsignedGreaterEqual;
                                } else {
                                    ASSERT(false, "Invalid type in GreaterThanEqual");
                                }

                                // Set cmp op
                                record.ops[2] = static_cast<uint64_t>(opCode);
                                block->AddRecord(record);
                            });
                            break;
                        }
                    }
                    break;
                }

                case IL::OpCode::IsNaN:
                case IL::OpCode::IsInf: {
                    // Resulting op code
                    DXILOpcodes dxilOpCode;

                    // Tested value
                    IL::ID value;

                    // Handle type
                    if (instr->opCode == IL::OpCode::IsNaN) {
                        value = instr->As<IL::IsNaNInstruction>()->value;
                        dxilOpCode = DXILOpcodes::IsNaN_;
                    } else {
                        value = instr->As<IL::IsInfInstruction>()->value;
                        dxilOpCode = DXILOpcodes::IsInf_;
                    }

                    // Handle as unary
                    UnaryOpSVOX(block, instr->result, value, [&](const Backend::IL::Type* type, IL::ID result, IL::ID value) {
                        uint64_t ops[2];

                        // Get intrinsic
                        const DXILFunctionDeclaration *intrinsic;
                        switch (type->As<Backend::IL::FPType>()->bitWidth) {
                            default:
                                ASSERT(false, "Invalid bit width");
                                return;
                            case 16:
                                intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpIsSpecialFloatF16);
                                break;
                            case 32:
                                intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpIsSpecialFloatF32);
                                break;
                        }

                        // Opcode
                        ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                            Backend::IL::IntConstant{.value = static_cast<uint32_t>(dxilOpCode)}
                        )->id);

                        // Value test
                        ops[1] = table.idRemapper.EncodeRedirectedUserOperand(value);

                        // Invoke into result
                        block->AddRecord(CompileIntrinsicCall(result, intrinsic, 2, ops));
                    });
                    break;
                }

                case IL::OpCode::KernelValue: {
                    auto _instr = instr->As<IL::KernelValueInstruction>();

                    // Handle value
                    switch (_instr->value) {
                        default: {
                            ASSERT(false, "Invalid value");
                            break;
                        }
                        case Backend::IL::KernelValue::DispatchThreadID: {
                            const DXILFunctionDeclaration *intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpThreadI32);

                            // Get each dimension
                            IL::ID threadIds[3];
                            for (uint32_t i = 0; i < 3; i++) {
                                threadIds[i] = program.GetIdentifierMap().AllocID();

                                // Get thread id at axis
                                uint64_t ops[2];
                                ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(static_cast<uint32_t>(DXILOpcodes::ThreadId))->id);
                                ops[1] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(i)->id);
                                block->AddRecord(CompileIntrinsicCall(threadIds[i], intrinsic, 2, ops));
                            }

                            // Create svox
                            table.idRemapper.SetUserRedirect(instr->result, AllocateSVOSequential(3, threadIds[0], threadIds[1], threadIds[2]));
                            break;
                        }
                    }
                    break;
                }
                
                case IL::OpCode::Extended: {
                    auto _instr = instr->As<IL::ExtendedInstruction>();

                    // Target intrinsic
                    const DXILFunctionDeclaration* intrinsic{nullptr};
                    
                    // The selected instruction
                    DXILOpcodes opCode;

                    // Handle value
                    switch (_instr->extendedOp) {
                        default: {
                            ASSERT(false, "Invalid extended opcode");
                            break;
                        }
                        case Backend::IL::ExtendedOp::Min: {
                            auto type = GetComponentType(program.GetTypeMap().GetType(_instr->operands[0]));
                            if (type->Is<Backend::IL::FPType>()) {
                                intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpBinaryF32);
                                opCode = DXILOpcodes::FMin_;
                            } else if (type->As<Backend::IL::IntType>()->signedness) {
                                intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpBinaryI32);
                                opCode = DXILOpcodes::IMin_;
                            } else {
                                intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpBinaryI32);
                                opCode = DXILOpcodes::UMin_;
                            }

                            BinaryOpSVOX(block, _instr->result, _instr->operands[0], _instr->operands[1], [&](const Backend::IL::Type*, IL::ID result, IL::ID a, IL::ID b) {
                                uint64_t ops[3];
                                ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(static_cast<uint32_t>(opCode))->id);
                                ops[1] = table.idRemapper.EncodeRedirectedUserOperand(a);
                                ops[2] = table.idRemapper.EncodeRedirectedUserOperand(b);
                                block->AddRecord(CompileIntrinsicCall(result, intrinsic, 3, ops));
                            });
                            break;
                        }
                        case Backend::IL::ExtendedOp::Max: {
                            auto type = GetComponentType(program.GetTypeMap().GetType(_instr->operands[0]));
                            if (type->Is<Backend::IL::FPType>()) {
                                intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpBinaryF32);
                                opCode = DXILOpcodes::FMax_;
                            } else if (type->As<Backend::IL::IntType>()->signedness) {
                                intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpBinaryI32);
                                opCode = DXILOpcodes::IMax_;
                            } else {
                                intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpBinaryI32);
                                opCode = DXILOpcodes::UMax_;
                            }

                            BinaryOpSVOX(block, _instr->result, _instr->operands[0], _instr->operands[1], [&](const Backend::IL::Type*, IL::ID result, IL::ID a, IL::ID b) {
                                uint64_t ops[3];
                                ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(static_cast<uint32_t>(opCode))->id);
                                ops[1] = table.idRemapper.EncodeRedirectedUserOperand(a);
                                ops[2] = table.idRemapper.EncodeRedirectedUserOperand(b);
                                block->AddRecord(CompileIntrinsicCall(result, intrinsic, 3, ops));
                            });
                            break;
                        }
                        case Backend::IL::ExtendedOp::Pow: {
                            intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpUnaryF32);
                            opCode = DXILOpcodes::Exp_;

                            // Implement pow(a, b) as exp(log(a) * b)
                            IL::ID logResult = program.GetIdentifierMap().AllocID();
                            IL::ID logMulResult = program.GetIdentifierMap().AllocID();

                            // log(x)
                            UnaryOpSVOX(block, logResult, _instr->operands[0], [&](const Backend::IL::Type*, IL::ID result, IL::ID value) {
                                uint64_t ops[2];
                                ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(static_cast<uint32_t>(DXILOpcodes::Log_))->id);
                                ops[1] = table.idRemapper.EncodeRedirectedUserOperand(value);
                                block->AddRecord(CompileIntrinsicCall(result, intrinsic, 2, ops));
                            });

                            // log(x) * b
                            BinaryOpSVOX(block, logMulResult, logResult, _instr->operands[1], [&](const Backend::IL::Type*, IL::ID result, IL::ID a, IL::ID b) {
                                record.SetUser(true, ~0u, result);
                                record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstBinOp);
                                record.opCount = 3;
                                record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                                record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(a);
                                record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(b);
                                record.ops[2] = static_cast<uint64_t>(LLVMBinOp::Mul);
                                block->AddRecord(record);
                            });

                            // exp(log(x) * b)
                            UnaryOpSVOX(block, _instr->result, logMulResult, [&](const Backend::IL::Type*, IL::ID result, IL::ID value) {
                                uint64_t ops[2];
                                ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(static_cast<uint32_t>(DXILOpcodes::Exp_))->id);
                                ops[1] = table.idRemapper.EncodeRedirectedUserOperand(value);
                                block->AddRecord(CompileIntrinsicCall(result, intrinsic, 2, ops));
                            });
                            break;
                        }
                        case Backend::IL::ExtendedOp::Exp: {
                            UnaryOpSVOX(block, _instr->result, _instr->operands[0], [&](const Backend::IL::Type*, IL::ID result, IL::ID value) {
                                uint64_t ops[2];
                                ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(static_cast<uint32_t>(DXILOpcodes::Exp_))->id);
                                ops[1] = table.idRemapper.EncodeRedirectedUserOperand(value);
                                block->AddRecord(CompileIntrinsicCall(result, table.intrinsics.GetIntrinsic(Intrinsics::DxOpUnaryF32), 2, ops));
                            });
                            break;
                        }
                        case Backend::IL::ExtendedOp::Floor: {
                            UnaryOpSVOX(block, _instr->result, _instr->operands[0], [&](const Backend::IL::Type*, IL::ID result, IL::ID value) {
                                uint64_t ops[2];
                                ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(static_cast<uint32_t>(DXILOpcodes::Round_ni_))->id);
                                ops[1] = table.idRemapper.EncodeRedirectedUserOperand(value);
                                block->AddRecord(CompileIntrinsicCall(result, table.intrinsics.GetIntrinsic(Intrinsics::DxOpUnaryF32), 2, ops));
                            });
                            break;
                        }
                        case Backend::IL::ExtendedOp::Ceil: {
                            UnaryOpSVOX(block, _instr->result, _instr->operands[0], [&](const Backend::IL::Type*, IL::ID result, IL::ID value) {
                                uint64_t ops[2];
                                ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(static_cast<uint32_t>(DXILOpcodes::Round_pi_))->id);
                                ops[1] = table.idRemapper.EncodeRedirectedUserOperand(value);
                                block->AddRecord(CompileIntrinsicCall(result, table.intrinsics.GetIntrinsic(Intrinsics::DxOpUnaryF32), 2, ops));
                            });
                            break;
                        }
                        case Backend::IL::ExtendedOp::Round: {
                            UnaryOpSVOX(block, _instr->result, _instr->operands[0], [&](const Backend::IL::Type*, IL::ID result, IL::ID value) {
                                uint64_t ops[2];
                                ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(static_cast<uint32_t>(DXILOpcodes::Round_ne_))->id);
                                ops[1] = table.idRemapper.EncodeRedirectedUserOperand(value);
                                block->AddRecord(CompileIntrinsicCall(result, table.intrinsics.GetIntrinsic(Intrinsics::DxOpUnaryF32), 2, ops));
                            });
                            break;
                        }
                        case Backend::IL::ExtendedOp::Sqrt: {
                            UnaryOpSVOX(block, _instr->result, _instr->operands[0], [&](const Backend::IL::Type*, IL::ID result, IL::ID value) {
                                uint64_t ops[2];
                                ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(static_cast<uint32_t>(DXILOpcodes::Sqrt_))->id);
                                ops[1] = table.idRemapper.EncodeRedirectedUserOperand(value);
                                block->AddRecord(CompileIntrinsicCall(result, table.intrinsics.GetIntrinsic(Intrinsics::DxOpUnaryF32), 2, ops));
                            });
                            break;
                        }
                        case Backend::IL::ExtendedOp::Abs: {
                            UnaryOpSVOX(block, _instr->result, _instr->operands[0], [&](const Backend::IL::Type*, IL::ID result, IL::ID value) {
                                uint64_t ops[2];
                                ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(static_cast<uint32_t>(DXILOpcodes::FAbs_))->id);
                                ops[1] = table.idRemapper.EncodeRedirectedUserOperand(value);
                                block->AddRecord(CompileIntrinsicCall(result, table.intrinsics.GetIntrinsic(Intrinsics::DxOpUnaryF32), 2, ops));
                            });
                            break;
                        }
                        case Backend::IL::ExtendedOp::FirstBitLow: {
                            UnaryOpSVOX(block, _instr->result, _instr->operands[0], [&](const Backend::IL::Type*, IL::ID result, IL::ID value) {
                                uint64_t ops[2];
                                ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(static_cast<uint32_t>(DXILOpcodes::FirstbitLo_))->id);
                                ops[1] = table.idRemapper.EncodeRedirectedUserOperand(value);
                                block->AddRecord(CompileIntrinsicCall(result, table.intrinsics.GetIntrinsic(Intrinsics::DxOpUnaryBitsI32), 2, ops));
                            });
                            break;
                        }
                        case Backend::IL::ExtendedOp::FirstBitHigh: {
                            IL::ID firstBitResult = program.GetIdentifierMap().AllocID();
                            IL::ID constant31     = program.GetConstants().UInt(31)->id;

                            // Select opcode
                            auto type = GetComponentType(program.GetTypeMap().GetType(_instr->operands[0]));
                            if (type->As<Backend::IL::IntType>()->signedness) {
                                opCode = DXILOpcodes::FirstbitSHi_;
                            } else {
                                opCode = DXILOpcodes::FirstbitHi_;
                            }
                            
                            // FirstBitHigh
                            UnaryOpSVOX(block, firstBitResult, _instr->operands[0], [&](const Backend::IL::Type*, IL::ID result, IL::ID value) {
                                uint64_t ops[2];
                                ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(static_cast<uint32_t>(opCode))->id);
                                ops[1] = table.idRemapper.EncodeRedirectedUserOperand(value);
                                block->AddRecord(CompileIntrinsicCall(result, table.intrinsics.GetIntrinsic(Intrinsics::DxOpUnaryBitsI32), 2, ops));
                            });

                            // 31 - FirstBitHigh
                            UnaryOpSVOX(block, _instr->result, firstBitResult, [&](const Backend::IL::Type*, IL::ID result, IL::ID b) {
                                record.SetUser(true, ~0u, result);
                                record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstBinOp);
                                record.opCount = 3;
                                record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                                record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(constant31);
                                record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(b);
                                record.ops[2] = static_cast<uint64_t>(LLVMBinOp::Sub);
                                block->AddRecord(record);
                            });
                            
                            opCode = DXILOpcodes::FirstbitHi_;
                            break;
                        }
                    }
                    break;
                }

                case IL::OpCode::Select: {
                    auto _instr = instr->As<IL::SelectInstruction>();

                    // Prepare record
                    record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstVSelect);
                    record.opCount = 3;
                    record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                    record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->pass);
                    record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->fail);
                    record.ops[2] = table.idRemapper.EncodeRedirectedUserOperand(_instr->condition);
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
                case IL::OpCode::BitCast: {
                    auto _instr = instr->As<IL::BitCastInstruction>();

                    // Get types
                    const Backend::IL::Type* valueType = typeMap.GetType(_instr->value);
                    const Backend::IL::Type* resultType = typeMap.GetType(_instr->result);

                    // LLVM IR does not differentiate between signed and unsigned, and is instead part of the instructions themselves (Fx. SDiv, UDiv)
                    // So, the resulting type will dictate future operations, value redirection is enough.
                    const bool bIsIntegerCast = Backend::IL::IsComponentType<Backend::IL::IntType>(valueType) && Backend::IL::IsComponentType<Backend::IL::IntType>(resultType);

                    // Any need to cast at all?
                    if (valueType == resultType || bIsIntegerCast) {
                        // Same, just redirect
                        table.idRemapper.SetUserRedirect(instr->result, _instr->value);
                    } else {
                        // Handle as unary
                        UnaryOpSVOX(block, _instr->result, _instr->value, [&](const Backend::IL::Type* type, IL::ID result, IL::ID value) {
                            // Prepare record
                            record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstCast);
                            record.opCount = 3;
                            record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(value);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(table.type.typeMap.GetType(resultType));
                            record.ops[2] = static_cast<uint64_t>(LLVMCastOp::BitCast);
                            block->AddRecord(record);
                        });
                    }
                    break;
                }
                case IL::OpCode::Trunc:
                case IL::OpCode::FloatToInt:
                case IL::OpCode::IntToFloat: {
                    // Prepare record
                    record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstCast);
                    record.opCount = 3;
                    record.ops = table.recordAllocator.AllocateArray<uint64_t>(3);

                    // Get types
                    const Backend::IL::Type* resultType = typeMap.GetType(instr->result);
                    
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
                    }

                    // Set result
                    record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(table.type.typeMap.GetType(resultType));

                    // Set cmp op
                    record.ops[2] = static_cast<uint64_t>(opCode);
                    block->AddRecord(record);
                    break;
                }

                case IL::OpCode::Any:
                case IL::OpCode::All: {
                    // Get value
                    IL::ID value;
                    if (instr->opCode == IL::OpCode::Any) {
                        value = instr->As<IL::AnyInstruction>()->value;
                    } else {
                        value = instr->As<IL::AllInstruction>()->value;
                    }

                    // Handle as SVOX
                    IterateSVOX(block, value, [&](const Backend::IL::Type* type, IL::ID id, uint32_t index, IL::ID max) {
                        IL::ID cmpId;

                        // Set comparison operation
                        switch (type->kind) {
                            default: {
                                ASSERT(false, "Invalid type");
                                break;
                            }
                            case Backend::IL::TypeKind::Bool: {
                                // Already in perfect form
                                cmpId = id;
                                break;
                            }
                            case Backend::IL::TypeKind::Int:
                            case Backend::IL::TypeKind::FP: {
                                // Allocate new id
                                cmpId = program.GetIdentifierMap().AllocID();

                                // Compare current component with zero
                                LLVMRecord cmpRecord{};
                                cmpRecord.SetUser(true, ~0u, cmpId);
                                cmpRecord.id = static_cast<uint32_t>(LLVMFunctionRecord::InstCmp);
                                cmpRecord.opCount = 3;
                                cmpRecord.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                                cmpRecord.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(id);

                                if (type->kind == Backend::IL::TypeKind::FP) {
                                    // Compare against 0.0f
                                    cmpRecord.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                                        type->As<Backend::IL::FPType>(),
                                        Backend::IL::FPConstant{.value = 0.0f}
                                    )->id);

                                    // Float comparison
                                    cmpRecord.ops[2] = static_cast<uint64_t>(LLVMCmpOp::FloatUnorderedNotEqual);
                                } else {
                                    // Compare against 0
                                    cmpRecord.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                                        type->As<Backend::IL::IntType>(),
                                        Backend::IL::IntConstant{.value = 0}
                                    )->id);

                                    // Integer comparison
                                    cmpRecord.ops[2] = static_cast<uint64_t>(LLVMCmpOp::IntNotEqual);
                                }

                                // Add op
                                block->AddRecord(cmpRecord);
                                break;
                            }
                        }

                        // First component?
                        if (!index) {
                            value = cmpId;
                            return;
                        }

                        // Allocate intermediate id
                        IL::ID pushValue = program.GetIdentifierMap().AllocID();

                        // BitAnd previous component, into temporary value
                        LLVMRecord andOp{};
                        andOp.SetUser(true, ~0u, pushValue);
                        andOp.id = static_cast<uint32_t>(LLVMFunctionRecord::InstBinOp);
                        andOp.opCount = 3;
                        andOp.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                        andOp.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(value);
                        andOp.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(cmpId);

                        // Set comparison mode
                        if (instr->opCode == IL::OpCode::Any) {
                            andOp.ops[2] = static_cast<uint64_t>(LLVMBinOp::Or);
                        } else {
                            andOp.ops[2] = static_cast<uint64_t>(LLVMBinOp::And);
                        }

                        // Add record
                        block->AddRecord(andOp);

                        // Set next
                        value = pushValue;
                    });

                    // Set final redirect
                    table.idRemapper.SetUserRedirect(instr->result, value);
                    break;
                }

                case IL::OpCode::ResourceToken: {
                    CompileResourceTokenInstruction(job, block, source, instr->As<IL::ResourceTokenInstruction>());
                    break;
                }

                case IL::OpCode::Export: {
                    CompileExportInstruction(block, instr->As<IL::ExportInstruction>());
                    break;
                }

                case IL::OpCode::AddressChain: {
                    auto _instr = instr->As<IL::AddressChainInstruction>();

                    // Get resulting type
                    const auto* pointerType = typeMap.GetType(_instr->result)->As<Backend::IL::PointerType>();

                    // Get type of the composite
                    const Backend::IL::Type* compositeType = program.GetTypeMap().GetType(_instr->composite);

                    // Resource indexing is handled in the using instruction
                    if (pointerType->addressSpace == Backend::IL::AddressSpace::Texture || pointerType->addressSpace == Backend::IL::AddressSpace::Buffer) {
                        break;
                    }

                    // Create record
                    record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstGEP);
                    record.opCount = 3 + _instr->chains.count;
                    record.ops = table.recordAllocator.AllocateArray<uint64_t>(record.opCount);
                    record.ops[0] = false;
                    record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(table.type.typeMap.GetType(compositeType));
                    record.ops[2] = table.idRemapper.EncodeRedirectedUserOperand(_instr->composite);

                    // Set chains
                    for (uint32_t i = 0; i < _instr->chains.count; i++) {
                        record.ops[3 + i] = table.idRemapper.EncodeRedirectedUserOperand(_instr->chains[i].index);
                    }

                    block->AddRecord(record);
                    break;
                }

                case IL::OpCode::Alloca: {
                    auto _instr = instr->As<IL::AllocaInstruction>();
                    
                    const auto* pointerType = typeMap.GetType(_instr->result)->As<Backend::IL::PointerType>();
                    
                    record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstAlloca);
                    record.opCount = 4u;
                    record.ops = table.recordAllocator.AllocateArray<uint64_t>(record.opCount);
                    record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(table.type.typeMap.GetType(pointerType));
                    record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(table.type.typeMap.GetType(program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{})));
                    record.ops[2] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(Backend::IL::GetPODNonAlignedTypeByteSize(pointerType->pointee))->id);
                    record.ops[3] = LLVMBitStreamWriter::EncodeSigned(4);
                    block->AddRecord(record);
                    break;
                }

                case IL::OpCode::Load: {
                    auto _instr = instr->As<IL::LoadInstruction>();

                    // Get type
                    const auto* pointerType = typeMap.GetType(_instr->address)->As<Backend::IL::PointerType>();

                    switch (pointerType->addressSpace) {
                        default: {
                            ASSERT(false, "Not implemented");
                            break;
                        }
                        case Backend::IL::AddressSpace::Function: {
                            record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstLoad);
                            record.opCount = 4u;
                            record.ops = table.recordAllocator.AllocateArray<uint64_t>(record.opCount);
                            record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->address);
                            record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(table.type.typeMap.GetType(pointerType->pointee));
                            record.ops[2] = LLVMBitStreamWriter::EncodeSigned(4);
                            record.ops[3] = false;
                            block->AddRecord(record);
                            break;
                        }
                        case Backend::IL::AddressSpace::Resource: {
                            ASSERT(pointerType->pointee->kind == Backend::IL::TypeKind::Buffer || pointerType->pointee->kind == Backend::IL::TypeKind::Texture, "Unexpected resource load");
                            
                            // The IL abstraction exposes resource handles as "pointers" for inclusive conformity,
                            // however, DXIL handles have no such concept. Just "assume" they were loaded, and let
                            // the succeeding instruction deal with the assumption.
                            table.idRemapper.SetUserRedirect(instr->result, _instr->address);
                            break;
                        }
                    }

                    break;
                }

                case IL::OpCode::Store: {
                    auto _instr = instr->As<IL::StoreInstruction>();

                    // Validate address space
                    const auto* pointerType = typeMap.GetType(_instr->address)->As<Backend::IL::PointerType>();
                    ASSERT(pointerType->addressSpace == Backend::IL::AddressSpace::Function, "Non function space stores not supported");

                    record.id = static_cast<uint32_t>(LLVMFunctionRecord::InstStore);
                    record.opCount = 4u;
                    record.ops = table.recordAllocator.AllocateArray<uint64_t>(record.opCount);
                    record.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->address);
                    record.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->value);
                    record.ops[2] = LLVMBitStreamWriter::EncodeSigned(4);
                    record.ops[3] = false;
                    block->AddRecord(record);

                    break;
                }

                case IL::OpCode::LoadBuffer: {
                    auto _instr = instr->As<IL::LoadBufferInstruction>();

                    // Get type
                    const auto* bufferType = typeMap.GetType(_instr->buffer)->As<Backend::IL::BufferType>();

                    // Type used for intrinsic
                    const Backend::IL::Type* elementType = bufferType->elementType;

                    // Mutate element type on structured
                    if (auto _struct = elementType->Cast<Backend::IL::StructType>()) {
                        ASSERT(_instr->offset != IL::InvalidID, "Offset on non-structured type");

                        // Get offset
                        auto offset = program.GetConstants().GetConstant(_instr->offset)->As<Backend::IL::IntConstant>();

                        // Get the element type
                        elementType = Backend::IL::GetStructuredTypeAtOffset(_struct, offset->value);
                        ASSERT(elementType, "Failed to deduce element type from offset");
                    }

                    // Get intrinsic
                    const DXILFunctionDeclaration *intrinsic;
                    switch (Backend::IL::GetComponentType(elementType)->kind) {
                        default:
                            ASSERT(false, "Invalid buffer element type");
                            return;
                        case Backend::IL::TypeKind::Int:
                            intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpBufferLoadI32);
                            break;
                        case Backend::IL::TypeKind::FP:
                            intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpBufferLoadF32);
                            break;
                    }

                    uint64_t ops[4];

                    // Opcode
                    ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                        Backend::IL::IntConstant{.value = static_cast<uint32_t>(DXILOpcodes::BufferLoad)}
                    )->id);

                    // Handle
                    ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->buffer);

                    // C0
                    ops[2] = table.idRemapper.EncodeRedirectedUserOperand(_instr->index);

                    // C1
                    if (_instr->offset != IL::InvalidID) {
                        ops[3] = table.idRemapper.EncodeRedirectedUserOperand(_instr->offset);
                    } else {
                        ops[3] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                            Backend::IL::UndefConstant{}
                        )->id);
                    }

                    // Invoke into result
                    block->AddRecord(CompileIntrinsicCall(_instr->result, intrinsic, 4, ops));

                    // Set as VOS
                    table.idRemapper.AllocSourceUserMapping(_instr->result, DXILIDUserType::VectorOnStruct, 0);
                    break;
                }

                case IL::OpCode::StoreBuffer: {
                    auto _instr = instr->As<IL::StoreBufferInstruction>();

                    // Get type
                    const auto* bufferType = typeMap.GetType(_instr->buffer)->As<Backend::IL::BufferType>();

                    // Get intrinsic
                    const DXILFunctionDeclaration *intrinsic;
                    switch (Backend::IL::GetComponentType(bufferType->elementType)->kind) {
                        default:
                            ASSERT(false, "Invalid buffer element type");
                            return;
                        case Backend::IL::TypeKind::Int:
                            intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpBufferStoreI32);
                            break;
                        case Backend::IL::TypeKind::FP:
                            intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpBufferStoreF32);
                            break;
                    }

                    uint64_t ops[9];

                    // Opcode
                    ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                        Backend::IL::IntConstant{.value = static_cast<uint32_t>(DXILOpcodes::BufferStore)}
                    )->id);

                    // Handle
                    ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->buffer);

                    // C0
                    ops[2] = table.idRemapper.EncodeRedirectedUserOperand(_instr->index);

                    // C1
                    ops[3] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                        Backend::IL::UndefConstant{}
                    )->id);

                    // Get component count
                    uint32_t count = GetSVOXCount(_instr->value);

                    // Visit all cases
                    for (uint32_t i = 0; i < 4u; i++) {
                        // Repeat last SVOX element if none remain
                        SVOXElement element = ExtractSVOXElement(block, _instr->value, std::min(i, count - 1));
                        ops[4 + i] = table.idRemapper.EncodeRedirectedUserOperand(element.value);
                    }

                    // Write mask
                    ops[8] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=8, .signedness=true}),
                        Backend::IL::IntConstant{.value = static_cast<uint32_t>(IL::ComponentMask::All)}
                    )->id);

                    // Invoke into result
                    block->AddRecord(CompileIntrinsicCall(_instr->result, intrinsic, 9, ops));
                    break;
                }

                case IL::OpCode::StoreTexture: {
                    auto _instr = instr->As<IL::StoreTextureInstruction>();

                    // Get type
                    const auto* textureType = typeMap.GetType(_instr->texture)->As<Backend::IL::TextureType>();

                    // Get component type
                    const Backend::IL::Type* componentType = Backend::IL::GetComponentType(textureType->sampledType);

                    // Get intrinsic
                    const DXILFunctionDeclaration *intrinsic;
                    switch (componentType->kind) {
                        default:
                        ASSERT(false, "Invalid buffer element type");
                            return;
                        case Backend::IL::TypeKind::Int: {
                            const auto* intType = componentType->As<Backend::IL::IntType>();
                            switch (intType->bitWidth) {
                                default:
                                    ASSERT(false, "Unsupported bit-width");
                                    break;
                                case 32:
                                    intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpTextureStoreI32);
                                    break;
                            }
                            break;
                        }
                        case Backend::IL::TypeKind::FP: {
                            const auto* fpType = componentType->As<Backend::IL::FPType>();
                            switch (fpType->bitWidth) {
                                default:
                                    ASSERT(false, "Unsupported bit-width");
                                    break;
                                case 16:
                                    intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpTextureStoreF16);
                                    break;
                                case 32:
                                    intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpTextureStoreF32);
                                    break;
                            }
                            break;
                        }
                    }

                    uint64_t ops[10];

                    // Opcode
                    ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                        Backend::IL::IntConstant{.value = static_cast<uint32_t>(DXILOpcodes::TextureStore)}
                    )->id);

                    // Handle
                    ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->texture);

                    // Get component counts
                    uint32_t indexCount = GetSVOXCount(_instr->index);
                    uint32_t texelCount = GetSVOXCount(_instr->texel);

                    // Undefined value
                    uint64_t undefIntConstant = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=false}),
                        Backend::IL::UndefConstant{}
                    )->id);

                    // Undefined value
                    uint64_t nullChannelConstant = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                        componentType,
                        Backend::IL::NullConstant{}
                    )->id);

                    // C0,1,2
                    ops[2] = indexCount > 0 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->index, 0).value) : undefIntConstant;
                    ops[3] = indexCount > 1 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->index, 1).value) : undefIntConstant;
                    ops[4] = indexCount > 2 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->index, 2).value) : undefIntConstant;

                    // V0,1,2
                    ops[5] = texelCount > 0 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->texel, 0).value) : nullChannelConstant;
                    ops[6] = texelCount > 1 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->texel, 1).value) : nullChannelConstant;
                    ops[7] = texelCount > 2 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->texel, 2).value) : nullChannelConstant;
                    ops[8] = texelCount > 3 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->texel, 3).value) : nullChannelConstant;

                    // Write mask
                    ops[9] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=8, .signedness=true}),
                        Backend::IL::IntConstant{.value = static_cast<uint32_t>(_instr->mask.value)}
                    )->id);

                    // Invoke into result
                    block->AddRecord(CompileIntrinsicCall(_instr->result, intrinsic, 10, ops));
                    break;
                }

                case IL::OpCode::LoadTexture: {
                    auto _instr = instr->As<IL::LoadTextureInstruction>();

                    // Get type
                    const auto* textureType = typeMap.GetType(_instr->texture)->As<Backend::IL::TextureType>();

                    // Get component type
                    const Backend::IL::Type* componentType = Backend::IL::GetComponentType(textureType->sampledType);

                    // Get intrinsic
                    const DXILFunctionDeclaration *intrinsic;
                    switch (componentType->kind) {
                        default:
                            ASSERT(false, "Invalid buffer element type");
                            return;
                        case Backend::IL::TypeKind::Int: {
                            const auto* intType = componentType->As<Backend::IL::IntType>();
                            switch (intType->bitWidth) {
                                default:
                                    ASSERT(false, "Unsupported bit-width");
                                    break;
                                case 32:
                                    intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpTextureLoadI32);
                                    break;
                            }
                            break;
                        }
                        case Backend::IL::TypeKind::FP: {
                            const auto* fpType = componentType->As<Backend::IL::FPType>();
                            switch (fpType->bitWidth) {
                                default:
                                    ASSERT(false, "Unsupported bit-width");
                                    break;
                                case 16:
                                    intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpTextureLoadF16);
                                    break;
                                case 32:
                                    intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpTextureLoadF32);
                                    break;
                            }
                            break;
                        }
                    }

                    uint64_t ops[9];

                    // Opcode
                    ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                        Backend::IL::IntConstant{.value = static_cast<uint32_t>(DXILOpcodes::TextureLoad)}
                    )->id);

                    // Handle
                    ops[1] = table.idRemapper.EncodeRedirectedUserOperand(_instr->texture);

                    // Mip
                    if (_instr->mip == IL::InvalidID) {
                        // Unassigned mips are undef values in DXIL
                        ops[2] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=false}),
                            Backend::IL::UndefConstant{}
                        )->id);
                    } else {
                        ops[2] = table.idRemapper.EncodeRedirectedUserOperand(_instr->mip);
                    }

                    // Get component counts
                    uint32_t indexCount  = GetSVOXCount(_instr->index);
                    uint32_t offsetCount = GetSVOXCount(_instr->offset);

                    // Undefined value
                    uint64_t undefIntConstant = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=false}),
                        Backend::IL::UndefConstant{}
                    )->id);

                    // C0,1,2
                    ops[3] = indexCount > 0 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->index, 0).value) : undefIntConstant;
                    ops[4] = indexCount > 1 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->index, 1).value) : undefIntConstant;
                    ops[5] = indexCount > 2 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->index, 2).value) : undefIntConstant;

                    // O0,1,2
                    ops[6] = offsetCount > 0 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->offset, 0).value) : undefIntConstant;
                    ops[7] = offsetCount > 1 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->offset, 1).value) : undefIntConstant;
                    ops[8] = offsetCount > 2 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->offset, 2).value) : undefIntConstant;

                    // Invoke into result
                    block->AddRecord(CompileIntrinsicCall(_instr->result, intrinsic, 9, ops));
                    break;
                }

                case IL::OpCode::SampleTexture: {
                    auto _instr = instr->As<IL::SampleTextureInstruction>();

                    // Get type
                    const auto* textureType = typeMap.GetType(_instr->texture)->As<Backend::IL::TextureType>();

                    // Get bit-width
                    uint32_t bitWidth = Backend::IL::GetComponentType(textureType->sampledType)->As<Backend::IL::FPType>()->bitWidth;
                    ASSERT(bitWidth == 16 || bitWidth == 32, "Unsupported sampling operation");

                    // Final op code
                    DXILOpcodes opcode;

                    // Final intrinsic
                    const DXILFunctionDeclaration *intrinsic;

                    // Select intrinsic and op-code
                    if (_instr->bias != IL::InvalidID) {
                        intrinsic = table.intrinsics.GetIntrinsic(bitWidth == 16 ? Intrinsics::DxOpSampleBiasF16 : Intrinsics::DxOpSampleBiasF32);
                        opcode = DXILOpcodes::SampleBias;
                    } else if (_instr->lod != IL::InvalidID) {
                        intrinsic = table.intrinsics.GetIntrinsic(bitWidth == 16 ? Intrinsics::DxOpSampleLevelF16 : Intrinsics::DxOpSampleLevelF32);
                        opcode = DXILOpcodes::SampleLevel;
                    } else if (_instr->ddx != IL::InvalidID) {
                        intrinsic = table.intrinsics.GetIntrinsic(bitWidth == 16 ? Intrinsics::DxOpSampleGradF16 : Intrinsics::DxOpSampleGradF32);
                        opcode = DXILOpcodes::SampleGrad;
                    } else {
                        intrinsic = table.intrinsics.GetIntrinsic(bitWidth == 16 ? Intrinsics::DxOpSampleF16 : Intrinsics::DxOpSampleF32);
                        opcode = DXILOpcodes::Sample;
                    }

                    // Optional, source record
                    const LLVMRecord* sourceRecord = _instr->source.IsValid() ? &source[_instr->source.codeOffset] : nullptr;

                    // Get original opcode, if possible
                    DXILOpcodes sourceOpcode{};
                    if (sourceRecord) {
                        sourceOpcode = static_cast<DXILOpcodes>(program.GetConstants().GetConstant<IL::IntConstant>(table.idMap.GetMappedRelative(
                            sourceRecord->sourceAnchor, sourceRecord->Op32(4)
                        ))->value);
                    }

                    switch (_instr->sampleMode) {
                        default:
                            ASSERT(false, "Unexpected sample mode");
                            break;
                        case Backend::IL::TextureSampleMode::Default: {
                            if (_instr->bias != IL::InvalidID) {
                                intrinsic = table.intrinsics.GetIntrinsic(bitWidth == 16 ? Intrinsics::DxOpSampleBiasF16 : Intrinsics::DxOpSampleBiasF32);
                                opcode = DXILOpcodes::SampleBias;
                            } else if (_instr->lod != IL::InvalidID) {
                                intrinsic = table.intrinsics.GetIntrinsic(bitWidth == 16 ? Intrinsics::DxOpSampleLevelF16 : Intrinsics::DxOpSampleLevelF32);
                                opcode = DXILOpcodes::SampleLevel;
                            } else if (_instr->ddx != IL::InvalidID) {
                                intrinsic = table.intrinsics.GetIntrinsic(bitWidth == 16 ? Intrinsics::DxOpSampleGradF16 : Intrinsics::DxOpSampleGradF32);
                                opcode = DXILOpcodes::SampleGrad;
                            } else {
                                intrinsic = table.intrinsics.GetIntrinsic(bitWidth == 16 ? Intrinsics::DxOpSampleF16 : Intrinsics::DxOpSampleF32);
                                opcode = DXILOpcodes::Sample;
                            }
                            break;
                        }
                        case Backend::IL::TextureSampleMode::DepthComparison: {
                            intrinsic = table.intrinsics.GetIntrinsic(bitWidth == 16 ? Intrinsics::DxOpSampleCmpF16 : Intrinsics::DxOpSampleCmpF32);
                            opcode = sourceOpcode == DXILOpcodes::SampleCmpLevelZero ? DXILOpcodes::SampleCmpLevelZero : DXILOpcodes::SampleCmp;
                            break;
                        }
                    }

                    TrivialStackVector<uint64_t, 16> ops;

                    // Opcode
                    ops.Add(table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                        Backend::IL::IntConstant{.value = static_cast<uint32_t>(opcode)}
                    )->id));

                    // Handle
                    ops.Add(table.idRemapper.EncodeRedirectedUserOperand(_instr->texture));

                    // Sampler
                    ops.Add(table.idRemapper.EncodeRedirectedUserOperand(_instr->sampler));

                    // Get component counts
                    uint32_t coordinateCount  = GetSVOXCount(_instr->coordinate);
                    uint32_t offsetCount = GetSVOXCount(_instr->offset);

                    // Undefined value
                    uint64_t undefFPConstant = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::FPType{.bitWidth=32}),
                        Backend::IL::UndefConstant{}
                    )->id);

                    // Undefined value
                    uint64_t undefIntConstant = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                        program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=false}),
                        Backend::IL::UndefConstant{}
                    )->id);

                    // C0,1,2
                    ops.Add(coordinateCount > 0 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->coordinate, 0).value) : undefFPConstant);
                    ops.Add(coordinateCount > 1 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->coordinate, 1).value) : undefFPConstant);
                    ops.Add(coordinateCount > 2 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->coordinate, 2).value) : undefFPConstant);
                    ops.Add(coordinateCount > 3 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->coordinate, 3).value) : undefFPConstant);

                    // O0,1,2
                    ops.Add(offsetCount > 0 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->offset, 0).value) : undefIntConstant);
                    ops.Add(offsetCount > 1 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->offset, 1).value) : undefIntConstant);
                    ops.Add(offsetCount > 2 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->offset, 2).value) : undefIntConstant);

                    // Handle additional operands
                    switch (opcode) {
                        default: {
                            ASSERT(false, "Unexpected sampling opcode");
                            break;
                        }
                        case DXILOpcodes::Sample: {
                            ops.Add(sourceOpcode == opcode ? sourceRecord->Op(4+10) : undefFPConstant);
                            break;
                        }
                        case DXILOpcodes::SampleBias: {
                            ops.Add(table.idRemapper.EncodeRedirectedUserOperand(_instr->bias));

                            // Clamp
                            ops.Add(sourceOpcode == opcode ? sourceRecord->Op(4+11) : undefFPConstant);
                            break;
                        }
                        case DXILOpcodes::SampleCmp: {
                            ops.Add(table.idRemapper.EncodeRedirectedUserOperand(_instr->reference));

                            // Clamp
                            ops.Add(sourceOpcode == opcode ? sourceRecord->Op(4+11) : undefFPConstant);
                            break;
                        }
                        case DXILOpcodes::SampleCmpLevelZero: {
                            ops.Add(table.idRemapper.EncodeRedirectedUserOperand(_instr->reference));
                            break;
                        }
                        case DXILOpcodes::SampleGrad: {
                            // Get component counts
                            uint32_t ddCount = GetSVOXCount(_instr->ddx);

                            // DDX
                            ops.Add(ddCount > 0 ?table.idRemapper.EncodeRedirectedUserOperand( ExtractSVOXElement(block, _instr->ddx, 0).value) : undefFPConstant);
                            ops.Add(ddCount > 1 ?table.idRemapper.EncodeRedirectedUserOperand( ExtractSVOXElement(block, _instr->ddx, 1).value) : undefFPConstant);
                            ops.Add(ddCount > 2 ?table.idRemapper.EncodeRedirectedUserOperand( ExtractSVOXElement(block, _instr->ddx, 2).value) : undefFPConstant);

                            // DDY
                            ops.Add(ddCount > 0 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->ddy, 0).value) : undefFPConstant);
                            ops.Add(ddCount > 1 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->ddy, 1).value) : undefFPConstant);
                            ops.Add(ddCount > 2 ? table.idRemapper.EncodeRedirectedUserOperand(ExtractSVOXElement(block, _instr->ddy, 2).value) : undefFPConstant);

                            // Clamp
                            ops.Add(sourceOpcode == opcode ? sourceRecord->Op(4+16) : undefFPConstant);
                            break;
                        }
                        case DXILOpcodes::SampleLevel: {
                            ops.Add(table.idRemapper.EncodeRedirectedUserOperand(_instr->lod));
                            break;
                        }
                    }

                    // Remap source derived data
                    for (uint64_t& op : ops) {
                        if (DXILIDRemapper::IsSourceOperand(op)) {
                            op = DXILIDRemapper::EncodeUserOperand(table.idMap.GetMappedRelative(sourceRecord->sourceAnchor, static_cast<uint32_t>(op)));
                        }
                    }

                    // Invoke into result
                    block->AddRecord(CompileIntrinsicCall(_instr->result, intrinsic, static_cast<uint32_t>(ops.Size()), ops.Data()));
                    break;
                }

                case IL::OpCode::Extract: {
                    auto _instr = instr->As<IL::ExtractInstruction>();

                    // DX12 backend only supports static extraction, for now
                    const IL::Constant* index = program.GetConstants().GetConstant(_instr->index);
                    ASSERT(index, "Dynamic extraction not supported");

                    // Assume int
                    auto offset = static_cast<uint32_t>(index->As<IL::IntConstant>()->value);
                    
                    // Emulated extraction, or real?
                    if (IsSVOX(_instr->composite)) {
                        // Source data may be SVOX
                        SVOXElement element = ExtractSVOXElement(block, _instr->composite, offset);
                        
                        // Point to the extracted element
                        table.idRemapper.SetUserRedirect(instr->result, element.value);
                    } else {
                        LLVMRecord recordExtract(LLVMFunctionRecord::InstExtractVal);
                        recordExtract.SetUser(true, ~0u, _instr->result);
                        recordExtract.opCount = 2;
                        recordExtract.ops = table.recordAllocator.AllocateArray<uint64_t>(2);
                        recordExtract.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(_instr->composite);
                        recordExtract.ops[1] = offset;
                        block->AddRecord(recordExtract);
                    }
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
#endif
            }
        }
    }

    // Add post record blocks
    for (const LLVMBlockElement &element: elements) {
        switch (static_cast<LLVMBlockElementType>(element.type)) {
            default: {
                break;
            }
            case LLVMBlockElementType::Block: {
                if (IsFunctionPostRecordDependentBlock(static_cast<LLVMReservedBlock>(block->blocks[element.id]->id))) {
                    block->elements.push_back(element);
                }
                break;
            }
        }
    }

    // Only create value segments if there's more than one function, no need to branch if not
    if (RequiresValueMapSegmentation()) {
        // Revert previous value
        table.idMap.Revert(declaration->segments.idSegment.head);
    }
}

void DXILPhysicalBlockFunction::CompileModuleFunction(LLVMRecord &record) {

}

void DXILPhysicalBlockFunction::StitchModuleFunction(LLVMRecord &record) {
    table.idRemapper.AllocRecordMapping(record);
}

void DXILPhysicalBlockFunction::StitchFunction(struct LLVMBlock *block) {
    // Get block
    FunctionBlock* functionBlock = GetFunctionBlock(block->uid);
    ASSERT(functionBlock, "Failed to deduce function block");

    // Definition order is linear to the internally linked functions
    DXILFunctionDeclaration *declaration = functions[internalLinkedFunctions[stitchFunctionIndex++]];

    // Branching handling for multi function setups
    if (RequiresValueMapSegmentation()) {
        // Merge the id value segment
        table.idMap.Merge(declaration->segments.idSegment);
    }

    // Handle constant relocation
    for (const DXILFunctionConstantRelocation& kv : declaration->segments.constantRelocationTable) {
        // Get stitched value index
        uint32_t stitchedConstant = table.idRemapper.GetUserMapping(kv.mapped);
        ASSERT(stitchedConstant != ~0u, "Invalid constant");

        // Set relocated source index
        table.idRemapper.SetSourceMapping(kv.sourceAnchor, stitchedConstant);
    }

    // Create snapshot
    DXILIDRemapper::StitchSnapshot idRemapperSnapshot = table.idRemapper.CreateStitchSnapshot();

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
            case LLVMReservedBlock::MetadataAttachment: {
                table.metadata.StitchMetadataAttachments(fnBlock, functionBlock->recordRelocation);
                break;
            }
        }
    }

    // Create parameter mappings
    for (uint32_t i = 0; i < declaration->parameters.Size(); i++) {
        table.idRemapper.AllocSourceMapping(declaration->parameters[i]);
    }

    // Visit function records, pass one, allocates results
    //   ? +1, Skip DeclareBlocks
    for (size_t recordIdx = 1; recordIdx < block->records.size(); recordIdx++) {
        LLVMRecord &record = block->records[recordIdx];

        // Current remapping anchor
        DXILIDRemapper::Anchor anchor = table.idRemapper.GetAnchor();

        // Allocate result
        if (record.hasValue) {
            table.idRemapper.AllocRecordMapping(record);
        }

        // Steal result as anchor
        record.result = anchor.stitchAnchor;
    }

    // Visit function records, pass two, stitches operands
    //   ? +1, Skip DeclareBlocks
    for (size_t recordIdx = 1; recordIdx < block->records.size(); recordIdx++) {
        LLVMRecord &record = block->records[recordIdx];

        // Setup writer
        DXILValueWriter writer(table, record);

        // Current remapping anchor
        DXILIDRemapper::Anchor anchor{record.result};

        // Handle instruction
        switch (static_cast<LLVMFunctionRecord>(record.id)) {
            default: {
                // If we reached this branch this is technically a failure
                ASSERT(false, "Unexpected record in stitch operation");
                
                // Force remap all operands as references
                for (uint32_t i = 0; i < record.opCount; i++) {
                    table.idRemapper.RemapRelative(anchor, record, record.ops[i]);
                }

                // OK
                break;
            }

            case LLVMFunctionRecord::InstExtractVal: {
                writer.RemapRelativeValue(anchor);
                break;
            }

            case LLVMFunctionRecord::InstInsertVal: {
                writer.RemapRelativeValue(anchor);
                writer.RemapRelativeValue(anchor);
                break;
            }

            case LLVMFunctionRecord::InstAtomicRW: {
                writer.RemapRelativeValue(anchor);
                writer.RemapRelative(anchor);
                break;
            }

            case LLVMFunctionRecord::InstGEP: {
                writer.Skip(2);

                for (uint32_t i = 2; i < record.opCount; i++) {
                    writer.RemapRelativeValue(anchor);
                }
                break;
            }

            case LLVMFunctionRecord::InstInBoundsGEP: {
                for (uint32_t i = 0; i < record.opCount; i++) {
                    writer.RemapRelativeValue(anchor);
                }
                break;
            }

            case LLVMFunctionRecord::InstBinOp: {
                writer.RemapRelativeValue(anchor);
                writer.RemapRelative(anchor);
                break;
            }

            case LLVMFunctionRecord::InstCast: {
                writer.RemapRelativeValue(anchor);
                break;
            }

            case LLVMFunctionRecord::InstVSelect: {
                writer.RemapRelativeValue(anchor);
                writer.RemapRelative(anchor);
                writer.RemapRelativeValue(anchor);
                break;
            }

            case LLVMFunctionRecord::InstCmp:
            case LLVMFunctionRecord::InstCmp2: {
                writer.RemapRelativeValue(anchor);
                writer.RemapRelative(anchor);
                break;
            }

            case LLVMFunctionRecord::InstRet: {
                if (record.opCount) {
                    writer.RemapRelativeValue(anchor);
                }
                break;
            }
            case LLVMFunctionRecord::InstBr: {
                if (record.opCount > 1) {
                    writer.Skip(2);
                    writer.RemapRelative(anchor);
                }
                break;
            }
            case LLVMFunctionRecord::InstSwitch: {
                writer.Skip(1);
                writer.RemapRelative(anchor);

                for (uint32_t i = 3; i < record.opCount; i += 2) {
                    table.idRemapper.Remap(record.ops[i]);
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
                table.idRemapper.Remap(record.Op(2));
                break;
            }

            case LLVMFunctionRecord::InstLoad: {
                writer.RemapRelativeValue(anchor);
                break;
            }

            case LLVMFunctionRecord::InstStore: {
                writer.RemapRelativeValue(anchor);
                writer.RemapRelativeValue(anchor);
                break;
            }

            case LLVMFunctionRecord::InstStore2: {
                writer.RemapRelativeValue(anchor);
                writer.Skip(1);
                writer.RemapRelative(anchor);
                break;
            }

            case LLVMFunctionRecord::InstCall:
            case LLVMFunctionRecord::InstCall2: {
                writer.Skip(3);
                writer.RemapRelative(anchor);

                for (uint32_t i = 4; i < record.opCount; i++) {
                    writer.RemapRelative(anchor);
                }
                break;
            }
        }

        writer.Finalize();
    }

    // Fixup all forward references to their new value indices
    table.idRemapper.ResolveForwardReferences();

    // Branching handling for multi function setups
    if (RequiresValueMapSegmentation()) {
        // Revert previous value
        table.idMap.Revert(declaration->segments.idSegment.head);

        // Create id map segment
        declaration->segments.idRemapperStitchSegment = table.idRemapper.Branch(idRemapperSnapshot);
    }
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

    // Mark the declaration as used
    table.compliance.MarkAsUsed(decl);

    // OK
    return record;
}

void DXILPhysicalBlockFunction::CopyTo(DXILPhysicalBlockFunction &out) {
    out.functions = functions;
    out.internalLinkedFunctions = internalLinkedFunctions;
}

void DXILPhysicalBlockFunction::CreateExportHandle(const DXCompileJob &job, struct LLVMBlock *block) {
    // Allocate sharted counter
    exportCounterHandle = program.GetIdentifierMap().AllocID();

    // Create handle
    CreateUniversalHandle(
        block,
        exportCounterHandle,
        DXILShaderResourceClass::UAVs,
        table.bindingInfo.shaderExportHandleId,
        table.bindingInfo.bindingInfo.shaderExportBaseRegister
    );

    // Allocate all export streams
    for (uint32_t i = 0; i < job.streamCount; i++) {
        uint32_t streamHandle = exportStreamHandles.Add(program.GetIdentifierMap().AllocID());

        // Create handle
        CreateUniversalHandle(
            block,
            streamHandle,
            DXILShaderResourceClass::UAVs,
            table.bindingInfo.shaderExportHandleId,
            table.bindingInfo.bindingInfo.shaderExportBaseRegister + (i + 1)
        );
    }
}

const DXILFunctionDeclaration *DXILPhysicalBlockFunction::FindDeclaration(const std::string_view &view) {
    for (const DXILFunctionDeclaration *decl: functions) {
        if (table.symbol.GetValueString(static_cast<uint32_t>(decl->anchor)) == view) {
            return decl;
        }
    }

    return nullptr;
}

DXILFunctionDeclaration *DXILPhysicalBlockFunction::AddDeclaration(const DXILFunctionDeclaration &declaration) {
    return functions.Add(new (allocators, kAllocModuleDXIL) DXILFunctionDeclaration(declaration));
}

void DXILPhysicalBlockFunction::CreateUniversalHandle(struct LLVMBlock *block, uint32_t result, DXILShaderResourceClass _class, uint32_t handleId, uint32_t registerBase) {
    // Are we on SM6.6 or beyond?
    if (table.metadata.SatisfiesShadingModel(6, 6)) {
        const uint32_t bindingHandle = program.GetIdentifierMap().AllocID();

        // Allocate handle
        {
            // Get intrinsic
            const DXILFunctionDeclaration *intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpCreateHandleFromBinding);

            uint64_t ops[4];

            // OpCode
            ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                Backend::IL::IntConstant{.value = static_cast<uint32_t>(DXILOpcodes::CreateHandleFromBinding)}
                )->id);

            // Binding
            ops[1] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                table.intrinsics.resBind,
                Backend::IL::StructConstant{
                    .members = {
                        program.GetConstants().FindConstantOrAdd(table.intrinsics.i32Type, Backend::IL::IntConstant{.value = registerBase}),
                        program.GetConstants().FindConstantOrAdd(table.intrinsics.i32Type, Backend::IL::IntConstant{.value = registerBase}),
                        program.GetConstants().FindConstantOrAdd(table.intrinsics.i32Type, Backend::IL::IntConstant{.value = table.bindingInfo.bindingInfo.space}),
                        program.GetConstants().FindConstantOrAdd(table.intrinsics.i8Type, Backend::IL::IntConstant{.value = static_cast<uint32_t>(_class)})
                    }
                }
            )->id);

            // Base register
            ops[2] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                Backend::IL::IntConstant{.value = registerBase}
            )->id);

            // Non-unform
            ops[3] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                program.GetTypeMap().FindTypeOrAdd(Backend::IL::BoolType{}),
                Backend::IL::BoolConstant{.value = false}
            )->id);

            // Create SM6.6 handle
            block->AddRecord(CompileIntrinsicCall(bindingHandle, intrinsic, 4, ops));
        }

        // Annotate the handle
        {
            // Get intrinsic
            const DXILFunctionDeclaration *intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpAnnotateHandle);

            // Get the handle
            const DXILMetadataHandleEntry* entry = table.metadata.GetHandleFromMetadata(_class, handleId);

            // Populate resource properties
            DXILResourceProperties properties{};
            switch (_class) {
                default:
                    ASSERT(false, "Invalid class");
                    break;
                case DXILShaderResourceClass::SRVs:
                    properties.basic.shape = static_cast<uint8_t>(entry->srv.shape);
                    properties.typed.resource.componentType = static_cast<uint8_t>(entry->srv.componentType);
                    properties.typed.resource.componentCount = static_cast<uint8_t>(GetShapeComponentCount(entry->srv.shape));
                    properties.typed.resource.sampleCount = 1u;
                    break;
                case DXILShaderResourceClass::UAVs:
                    properties.basic.shape = static_cast<uint8_t>(entry->uav.shape);
                    properties.basic.isUAV = true;
                    properties.typed.resource.componentType = static_cast<uint8_t>(entry->uav.componentType);
                    properties.typed.resource.componentCount = static_cast<uint8_t>(GetShapeComponentCount(entry->uav.shape));
                    properties.typed.resource.sampleCount = 1u;
                    break;
                case DXILShaderResourceClass::CBVs:
                    properties.basic.shape = static_cast<uint8_t>(DXILShaderResourceShape::CBuffer);
                    properties.typed.cbufferByteSize = static_cast<uint32_t>(Backend::IL::GetPODNonAlignedTypeByteSize(entry->type->As<Backend::IL::PointerType>()->pointee));
                    break;
                case DXILShaderResourceClass::Samplers:
                    properties.basic.shape = static_cast<uint8_t>(DXILShaderResourceShape::Sampler);
                    break;
            }
            
            uint64_t ops[3];

            // OpCode
            ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                Backend::IL::IntConstant{.value = static_cast<uint32_t>(DXILOpcodes::AnnotateHandle)}
                )->id);

            // Handle
            ops[1] = table.idRemapper.EncodeRedirectedUserOperand(bindingHandle);

            // Properties
            ops[2] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                table.intrinsics.resourceProperties,
                Backend::IL::StructConstant{
                    .members = {
                        program.GetConstants().FindConstantOrAdd(table.intrinsics.i32Type, Backend::IL::IntConstant{.value = properties.basic.opaque}),
                        program.GetConstants().FindConstantOrAdd(table.intrinsics.i32Type, Backend::IL::IntConstant{.value = properties.typed.opaque})
                    }
                }
            )->id);

            // Create 6.6 annotation
            block->AddRecord(CompileIntrinsicCall(result, intrinsic, 3, ops));
        }
    } else {
        // Get intrinsic
        const DXILFunctionDeclaration *intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpCreateHandle);

        /*
         * DXIL Specification
         *   declare %dx.types.Handle @dx.op.createHandle(
         *       i32,                  ; opcode
         *       i8,                   ; resource class: SRV=0, UAV=1, CBV=2, Sampler=3
         *       i32,                  ; resource range ID (constant)
         *       i32,                  ; index into the range
         *       i1)                   ; non-uniform resource index: false or true
         */

        uint64_t ops[5];

        // OpCode
        ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::IntConstant{.value = static_cast<uint32_t>(DXILOpcodes::CreateHandle)}
        )->id);

        // Resource class
        ops[1] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=8, .signedness=true}),
            Backend::IL::IntConstant{.value = static_cast<uint32_t>(_class)}
        )->id);

        // Handle
        ops[2] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::IntConstant{.value = handleId}
        )->id);

        // Register base
        ops[3] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::IntConstant{.value = registerBase}
        )->id);

        // Non-uniform?
        ops[4] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::BoolType{}),
            Backend::IL::BoolConstant{.value = false}
        )->id);

        // Create shared resource prmt handle
        block->AddRecord(CompileIntrinsicCall(result, intrinsic, 5, ops));
    }
}

void DXILPhysicalBlockFunction::CreateHandles(const DXCompileJob &job, struct LLVMBlock *block) {
    CreateExportHandle(job, block);
    CreatePRMTHandle(job, block);
    CreateDescriptorHandle(job, block);
    CreateEventHandle(job, block);
    CreateConstantHandle(job, block);
    CreateShaderDataHandle(job, block);
}

void DXILPhysicalBlockFunction::CreatePRMTHandle(const DXCompileJob &job, struct LLVMBlock *block) {
    // Allocate sharted counter
    resourcePRMTHandle = program.GetIdentifierMap().AllocID();
    samplerPRMTHandle = program.GetIdentifierMap().AllocID();
    
    // Create resource handle
    CreateUniversalHandle(
        block,
        resourcePRMTHandle,
        DXILShaderResourceClass::SRVs,
        table.bindingInfo.resourcePRMTHandleId,
        table.bindingInfo.bindingInfo.resourcePRMTBaseRegister
    );

    // Create sampler handle
    CreateUniversalHandle(
        block,
        samplerPRMTHandle,
        DXILShaderResourceClass::SRVs,
        table.bindingInfo.samplerPRMTHandleId,
        table.bindingInfo.bindingInfo.samplerPRMTBaseRegister
    );
}

void DXILPhysicalBlockFunction::CreateDescriptorHandle(const DXCompileJob &job, struct LLVMBlock *block) {
    // Allocate sharted counter
    descriptorHandle = program.GetIdentifierMap().AllocID();
    
    // Create handle
    CreateUniversalHandle(
        block,
        descriptorHandle,
        DXILShaderResourceClass::CBVs,
        table.bindingInfo.descriptorConstantsHandleId,
        table.bindingInfo.bindingInfo.descriptorConstantBaseRegister
    );
}

void DXILPhysicalBlockFunction::CreateEventHandle(const DXCompileJob &job, struct LLVMBlock *block) {
    IL::ShaderDataMap& shaderDataMap = table.program.GetShaderDataMap();

    // Allocate sharted counter
    eventHandle = program.GetIdentifierMap().AllocID();
    
    // Create handle
    CreateUniversalHandle(
        block,
        eventHandle,
        DXILShaderResourceClass::CBVs,
        table.bindingInfo.eventConstantsHandleId,
        table.bindingInfo.bindingInfo.eventConstantBaseRegister
    );

    // Requested dword count
    uint32_t dwordCount = 0;

    // Aggregate dword count
    for (const ShaderDataInfo& info : shaderDataMap) {
        if (info.type == ShaderDataType::Event) {
            dwordCount++;
        }
    }

    // Number of effective rows
    uint32_t rowCount = (dwordCount + 3) / 4;

    // All rows for later swizzling
    TrivialStackVector<uint32_t, 16u> legacyRows(allocators);

    // Create all row loads
    for (uint32_t row = 0; row < rowCount; row++) {
        // Allocate ids
        uint32_t rowLegacyLoad = legacyRows.Add(program.GetIdentifierMap().AllocID());

        // Get intrinsic
        const DXILFunctionDeclaration *intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpCBufferLoadLegacyI32);

        /*
          *  ; overloads: SM5.1: f32|i32|f64,  future SM: possibly deprecated
          *    %dx.types.CBufRet.f32 = type { float, float, float, float }
          *    declare %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(
          *       i32,                  ; opcode
          *       %dx.types.Handle,     ; resource handle
          *       i32)                  ; 0-based row index (row = 16-byte DXBC register)
         */

        uint64_t ops[3];

        ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::IntConstant{.value = static_cast<uint32_t>(DXILOpcodes::CBufferLoadLegacy)}
        )->id);

        ops[1] = table.idRemapper.EncodeRedirectedUserOperand(eventHandle);

        ops[2] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::IntConstant{.value = static_cast<int64_t>(row)}
        )->id);

        // Invoke
        block->AddRecord(CompileIntrinsicCall(rowLegacyLoad, intrinsic, 3, ops));
    }

    // Current dword offset
    uint32_t dwordOffset = 0;

    // Create shader data mappings to handle
    for (const ShaderDataInfo& info : shaderDataMap) {
        if (info.type != ShaderDataType::Event) {
            continue;
        }

        // Get variable
        const Backend::IL::Variable* variable = shaderDataMap.Get(info.id);

        // Extract respective value
        LLVMRecord recordExtract(LLVMFunctionRecord::InstExtractVal);
        recordExtract.SetUser(true, ~0u, variable->id);
        recordExtract.opCount = 2;
        recordExtract.ops = table.recordAllocator.AllocateArray<uint64_t>(2);
        recordExtract.ops[0] = DXILIDRemapper::EncodeUserOperand(legacyRows[dwordOffset / 4]);
        recordExtract.ops[1] = dwordOffset % 4;
        block->AddRecord(recordExtract);

        // Next!
        dwordOffset++;
    }
}

void DXILPhysicalBlockFunction::CreateConstantHandle(const DXCompileJob &job, struct LLVMBlock *block) {
    IL::ShaderDataMap& shaderDataMap = table.program.GetShaderDataMap();

    // Allocate shared counter
    constantHandle = program.GetIdentifierMap().AllocID();
    
    // Create handle
    CreateUniversalHandle(
        block,
        constantHandle,
        DXILShaderResourceClass::CBVs,
        table.bindingInfo.shaderDataConstantsHandleId,
        table.bindingInfo.bindingInfo.shaderDataConstantRegister
    );

    // Requested dword count
    uint32_t dwordCount = 0;

    // Reserved prefix
    dwordCount += static_cast<uint32_t>(ReservedConstantDataDWords::Prefix);

    // Aggregate dword count
    for (const ShaderDataInfo& info : shaderDataMap) {
        if (info.type == ShaderDataType::Descriptor) {
            dwordCount += info.descriptor.dwordCount;
        }
    }

    // Number of effective rows
    uint32_t rowCount = (dwordCount + 3) / 4;

    // All rows for later swizzling
    TrivialStackVector<uint32_t, 16u> legacyRows(allocators);

    // Create all row loads
    for (uint32_t row = 0; row < rowCount; row++) {
        // Allocate ids
        uint32_t rowLegacyLoad = legacyRows.Add(program.GetIdentifierMap().AllocID());

        // Get intrinsic
        const DXILFunctionDeclaration *intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpCBufferLoadLegacyI32);

        /*
          *  ; overloads: SM5.1: f32|i32|f64,  future SM: possibly deprecated
          *    %dx.types.CBufRet.f32 = type { float, float, float, float }
          *    declare %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(
          *       i32,                  ; opcode
          *       %dx.types.Handle,     ; resource handle
          *       i32)                  ; 0-based row index (row = 16-byte DXBC register)
         */

        uint64_t ops[3];

        ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::IntConstant{.value = static_cast<uint32_t>(DXILOpcodes::CBufferLoadLegacy)}
        )->id);

        ops[1] = table.idRemapper.EncodeRedirectedUserOperand(constantHandle);

        ops[2] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::IntConstant{.value = static_cast<int64_t>(row)}
        )->id);

        // Invoke
        block->AddRecord(CompileIntrinsicCall(rowLegacyLoad, intrinsic, 3, ops));
    }

    // Current dword offset
    uint32_t dwordOffset = 0;

    // Get reserved values
    for (uint32_t i = 0; i < static_cast<uint32_t>(ReservedConstantDataDWords::Prefix); i++) {
        reservedConstantRange[i] = program.GetIdentifierMap().AllocID();
        
        // Extract respective value
        LLVMRecord recordExtract(LLVMFunctionRecord::InstExtractVal);
        recordExtract.SetUser(true, ~0u, reservedConstantRange[i]);
        recordExtract.opCount = 2;
        recordExtract.ops = table.recordAllocator.AllocateArray<uint64_t>(2);
        recordExtract.ops[0] = DXILIDRemapper::EncodeUserOperand(legacyRows[dwordOffset / 4]);
        recordExtract.ops[1] = dwordOffset % 4;
        block->AddRecord(recordExtract);

        // Next!
        dwordOffset++;
    }

    // Create shader data mappings to handle
    for (const ShaderDataInfo& info : shaderDataMap) {
        if (info.type != ShaderDataType::Descriptor) {
            continue;
        }

        // Get variable
        const Backend::IL::Variable* variable = shaderDataMap.Get(info.id);

        // Extract all dwords
        TrivialStackVector<IL::ID, 16> dwords;
        for (uint32_t i = 0; i < info.descriptor.dwordCount; i++) {
            IL::ID result;
            if (info.descriptor.dwordCount == 1) {
                result = variable->id;
            } else {
                result = program.GetIdentifierMap().AllocID();
            }
            
            // Extract respective value
            LLVMRecord recordExtract(LLVMFunctionRecord::InstExtractVal);
            recordExtract.SetUser(true, ~0u, result);
            recordExtract.opCount = 2;
            recordExtract.ops = table.recordAllocator.AllocateArray<uint64_t>(2);
            recordExtract.ops[0] = DXILIDRemapper::EncodeUserOperand(legacyRows[dwordOffset / 4]);
            recordExtract.ops[1] = dwordOffset % 4;
            block->AddRecord(recordExtract);
            dwords.Add(result);

            // Next!
            dwordOffset++;
        }

        // If multiple dwords, create aggregate
        if (info.descriptor.dwordCount > 1) {
            // Get pointee
            const Backend::IL::Type *pointee = variable->type;

            // Create struct from dwords
            table.idRemapper.SetUserRedirect(variable->id, AllocateSVOStructSequential(pointee, dwords.Data(), info.descriptor.dwordCount));
        }
    }

    // Validation
    ASSERT(dwordOffset == dwordCount, "DWord mismatch");
}

void DXILPhysicalBlockFunction::CreateShaderDataHandle(const DXCompileJob &job, struct LLVMBlock *block) {
    IL::ShaderDataMap& shaderDataMap = table.program.GetShaderDataMap();

    // Current offset
    uint32_t registerOffset = 0;

    // Create a handle per resource
    for (const ShaderDataInfo& info : shaderDataMap) {
        if (!(info.type & ShaderDataType::DescriptorMask)) {
            continue;
        }

        // Get variable
        const Backend::IL::Variable* variable = shaderDataMap.Get(info.id);

        // Create handle
        CreateUniversalHandle(
            block,
            variable->id,
            DXILShaderResourceClass::UAVs,
            table.bindingInfo.shaderDataHandleId + registerOffset,
            table.bindingInfo.bindingInfo.shaderResourceBaseRegister + registerOffset
        );

        // Next
        registerOffset++;
    }
}

DXILPhysicalBlockFunction::HandleMetadata DXILPhysicalBlockFunction::GetResourceHandleRecord(const Vector<LLVMRecord> &source, IL::ID resource) {
    // Get resource instruction
    IL::InstructionRef<> resourceInstr = program.GetIdentifierMap().Get(resource);
    ASSERT(resourceInstr->IsUserInstruction(), "Resource tokens not supported on custom fetching");

    // Get and validate record
    const LLVMRecord* resourceRecord = &source[resourceInstr->source.codeOffset];
    ASSERT(resourceRecord->Is(LLVMFunctionRecord::InstCall2), "Unexpected resource record");

    // Validate op code
    uint64_t opCode = program.GetConstants().GetConstant<IL::IntConstant>(table.idMap.GetMappedRelative(resourceRecord->sourceAnchor, resourceRecord->Op32(4)))->value;

    // Optional annotation
    DXILResourceProperties annotation{};

    // Is the top level an annotation handle?
    if (opCode == static_cast<uint32_t>(DXILOpcodes::AnnotateHandle)) {
        // Get the annotation constant
        auto constant = program.GetConstants().GetConstant(table.idMap.GetMappedRelative(resourceRecord->sourceAnchor, resourceRecord->Op32(6u)));

        // To property map
        switch (constant->kind) {
            default: {
                break;
            }
            case Backend::IL::ConstantKind::Struct: {
                auto _struct = constant->As<IL::StructConstant>();
                annotation.basic.opaque = static_cast<uint32_t>(_struct->members.at(0)->As<IL::IntConstant>()->value);
                annotation.typed.opaque = static_cast<uint32_t>(_struct->members.at(1)->As<IL::IntConstant>()->value);
                break;
            }
            case Backend::IL::ConstantKind::Null: {
                // Null constants are allowed
                break;
            }
        }

        // Get the declaration handle
        resourceInstr = program.GetIdentifierMap().Get(table.idMap.GetMappedRelative(resourceRecord->sourceAnchor, resourceRecord->Op32(5)));

        // Get and validate record
        resourceRecord = &source[resourceInstr->source.codeOffset];
        ASSERT(resourceRecord->Is(LLVMFunctionRecord::InstCall2), "Unexpected resource record");

        // Next op-code
        opCode = program.GetConstants().GetConstant<IL::IntConstant>(table.idMap.GetMappedRelative(resourceRecord->sourceAnchor, resourceRecord->Op32(4)))->value;
    }

    // Resulting metadata
    HandleMetadata metadata{};

    // Handle on declaration type
    switch (static_cast<DXILOpcodes>(opCode)) {
        default: {
            ASSERT(false, "Unexpected handle type");
            break;
        }
        case DXILOpcodes::CreateHandle: {
            // Get the class
            metadata._class = static_cast<DXILShaderResourceClass>(program.GetConstants().GetConstant<IL::IntConstant>(table.idMap.GetMappedRelative(resourceRecord->sourceAnchor, resourceRecord->Op32(5)))->value);
            
            // Range indices may be dynamic
            metadata.rangeConstantOrValue = table.idMap.GetMappedRelative(resourceRecord->sourceAnchor, resourceRecord->Op32(7));

            // Handle ids are always stored as constants
            uint32_t handleId = static_cast<uint32_t>(program.GetConstants().GetConstant<IL::IntConstant>(table.idMap.GetMappedRelative(resourceRecord->sourceAnchor, resourceRecord->Op32(6)))->value);

            // Get entry
            metadata.entry = table.metadata.GetHandle(metadata._class, handleId);
            break;
        }
        case DXILOpcodes::CreateHandleFromBinding: {
            // Get the binding constants
            auto bindings = program.GetConstants().GetConstant(table.idMap.GetMappedRelative(resourceRecord->sourceAnchor, resourceRecord->Op32(5u)));

            // Get binding operands
            int64_t rangeLowerBound{0};
            int64_t rangeUpperBound{0};
            int64_t spaceID{0};

            // May be null constant
            switch (bindings->kind) {
                default: {
                    ASSERT(false, "Invalid binding kind");
                    break;
                }
                case Backend::IL::ConstantKind::Struct: {
                    auto _struct = bindings->As<IL::StructConstant>();

                    // Get constants
                    ASSERT(_struct->members.size() == 4u, "Unexpected binding size");
                    rangeLowerBound = _struct->members[0]->As<Backend::IL::IntConstant>()->value;
                    rangeUpperBound = _struct->members[1]->As<Backend::IL::IntConstant>()->value;
                    spaceID = _struct->members[2]->As<Backend::IL::IntConstant>()->value;
                    metadata._class = static_cast<DXILShaderResourceClass>(_struct->members[3]->As<Backend::IL::IntConstant>()->value);
                    break;
                }
                case Backend::IL::ConstantKind::Null: {
                    // Null constants are allowed, SRV -> 0
                    metadata._class = DXILShaderResourceClass::SRVs;
                    break;
                }
            }

            // Get the actual handle type
            metadata.entry = table.metadata.GetHandle(
                metadata._class,
                spaceID,
                rangeLowerBound,
                rangeUpperBound
            );

            // Range indices may be dynamic
            metadata.rangeConstantOrValue = table.idMap.GetMappedRelative(resourceRecord->sourceAnchor, resourceRecord->Op32(6));
            break;
        }
        case DXILOpcodes::CreateHandleFromHeap: {
            // Translate class
            switch (static_cast<DXILShaderResourceShape>(annotation.basic.shape)) {
                default:
                    metadata._class = DXILShaderResourceClass::SRVs;
                    break;
                case DXILShaderResourceShape::Texture1D:
                case DXILShaderResourceShape::Texture2D:
                case DXILShaderResourceShape::Texture2DMS:
                case DXILShaderResourceShape::Texture3D:
                case DXILShaderResourceShape::TextureCube:
                case DXILShaderResourceShape::Texture1DArray:
                case DXILShaderResourceShape::Texture2DArray:
                case DXILShaderResourceShape::Texture2DMSArray:
                case DXILShaderResourceShape::TextureCubeArray:
                case DXILShaderResourceShape::TypedBuffer:
                case DXILShaderResourceShape::RawBuffer:
                case DXILShaderResourceShape::StructuredBuffer:
                    metadata._class = annotation.basic.isUAV ? DXILShaderResourceClass::UAVs : DXILShaderResourceClass::SRVs;
                    break;
                case DXILShaderResourceShape::CBuffer:
                    metadata._class = DXILShaderResourceClass::CBVs;
                    break;
                case DXILShaderResourceShape::Sampler:
                    metadata._class = DXILShaderResourceClass::Samplers;
                    break;
            }

            // No actual entry, just assume from the range value
            metadata.entry = nullptr;
            
            // Index
            metadata.rangeConstantOrValue = table.idMap.GetMappedRelative(resourceRecord->sourceAnchor, resourceRecord->Op32(5));
            break;
        }
    }

    // OK
    return metadata;
}

DXILPhysicalBlockFunction::DynamicRootSignatureUserMapping DXILPhysicalBlockFunction::GetResourceUserMapping(const DXCompileJob& job, const Vector<LLVMRecord>& source, IL::ID resource) {
    DynamicRootSignatureUserMapping out;

    // TODO: This will not hold true for everything

    // Get and validate record
    HandleMetadata metadata = GetResourceHandleRecord(source, resource);

    // Runtime only bindings just export the dynamic offset
    if (!metadata.entry) {
        out.dynamicOffset = metadata.rangeConstantOrValue;
        return out;
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

    // Compile time?
    uint32_t rangeIndex{~0u};
    if (auto constant = program.GetConstants().GetConstant<IL::IntConstant>(metadata.rangeConstantOrValue)) {
        rangeIndex = static_cast<uint32_t>(constant->value);
    } else {
        // Get runtime instruction
        IL::InstructionRef<> offsetInstr = program.GetIdentifierMap().Get(metadata.rangeConstantOrValue);
        if (!offsetInstr.Is<IL::AddInstruction>()) {
            return {};
        }

        // Get typed
        auto _offsetInstr = offsetInstr.As<IL::AddInstruction>();

        // Assume dynamic counterpart
        out.dynamicOffset = _offsetInstr->lhs;

        // Assume DXC style constant offset
        auto constantOffset = program.GetConstants().GetConstant<IL::IntConstant>(_offsetInstr->rhs);
        if (!constantOffset) {
            return {};
        }

        // Assume index from base range
        rangeIndex = static_cast<uint32_t>(constantOffset->value);
    }

    // Translate class
    RootSignatureUserClassType classType;
    switch (static_cast<DXILShaderResourceClass>(metadata._class)) {
        default:
            ASSERT(false, "Invalid class");
            return {};
        case DXILShaderResourceClass::SRVs:
            classType = RootSignatureUserClassType::SRV;
            break;
        case DXILShaderResourceClass::UAVs:
            classType = RootSignatureUserClassType::UAV;
            break;
        case DXILShaderResourceClass::CBVs:
            classType = RootSignatureUserClassType::CBV;
            break;
        case DXILShaderResourceClass::Samplers:
            classType = RootSignatureUserClassType::Sampler;
            break;
    }

    // Translate shading model to visibility
    RootParameterVisibility rootVisibility;
    switch (table.metadata.shadingModel._class) {
        default:
            ASSERT(false, "Invalid shading model");
            rootVisibility = RootParameterVisibility::Compute;
            break;
        case DXILShadingModelClass::CS:
            rootVisibility = RootParameterVisibility::Compute;
            break;
        case DXILShadingModelClass::VS:
            rootVisibility = RootParameterVisibility::Vertex;
            break;
        case DXILShadingModelClass::PS:
            rootVisibility = RootParameterVisibility::Pixel;
            break;
        case DXILShadingModelClass::GS:
            rootVisibility = RootParameterVisibility::Geometry;
            break;
        case DXILShadingModelClass::HS:
            rootVisibility = RootParameterVisibility::Hull;
            break;
        case DXILShadingModelClass::DS:
            rootVisibility = RootParameterVisibility::Domain;
            break;
    }

    // Get user space
    const RootSignatureVisibilityClass& visibilityClass = job.instrumentationKey.physicalMapping->visibility[static_cast<uint32_t>(rootVisibility)];
    const RootSignatureUserClass&       userClass       = visibilityClass.spaces[static_cast<uint32_t>(classType)];
    const RootSignatureUserSpace&       userSpace       = userClass.spaces.at(metadata.entry->bindSpace);

    // If the range index is beyond the accessible mappings, it implies arrays or similar
    if (rangeIndex > userSpace.lastRegister) {
        ASSERT(out.dynamicOffset == IL::InvalidID, "Dynamic mapping with out of bounds range index");

        // Validate that the end mapping is unbounded
        // Array descriptor ranges should be fully visible at this point
        ASSERT(userSpace.mappings.at(userSpace.lastRegister).isUnbounded, "Dynamic mapping from bounded range");

        // Effective distance
        const uint32_t distanceFromEnd = rangeIndex - userSpace.lastRegister;

        // Assign distance as the dynamic offset to validate
        out.dynamicOffset = program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::IntConstant{.value = static_cast<uint32_t>(distanceFromEnd)}
        )->id;

        // Set to last index
        rangeIndex = userSpace.lastRegister;
    }

    // Assign source
    out.source = &userSpace.mappings.at(rangeIndex);

    // OK
    return out;
}

void DXILPhysicalBlockFunction::CompileResourceTokenInstruction(const DXCompileJob& job, LLVMBlock* block, const Vector<LLVMRecord>& source, const IL::ResourceTokenInstruction* _instr) {
    DynamicRootSignatureUserMapping userMapping = GetResourceUserMapping(job, source, _instr->resource);
    ASSERT(userMapping.source || userMapping.dynamicOffset != IL::InvalidID, "Fallback user mappings not supported yet");

    // Total number of metadata dwords
    static constexpr uint32_t kMetadataDWordCount = static_cast<uint32_t>(Backend::IL::ResourceTokenMetadataField::Count);

    // Use shared representation
    auto tokenMetadataStruct = program.GetTypeMap().GetResourceToken();
    table.type.typeMap.GetType(tokenMetadataStruct);

    // All dwords
    TrivialStackVector<uint32_t, kMetadataDWordCount> metadataMap;
    
    // Static samplers are valid by default, however have no "real" data
    if (userMapping.source && userMapping.source->isStaticSampler) {
        // Assign packed token
        metadataMap.Add(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::IntConstant{.value = VirtualResourceMapping {
                ResourceToken {
                    .puid = 0,
                    .type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Sampler)
                }
            }.token.packedToken}
        )->id);

        // Just zero out the rest
        for (uint32_t i = 1; i < kMetadataDWordCount; i++) {
            metadataMap.Add(program.GetConstants().UInt(0)->id);
        }

        // Allocate the struct
        table.idRemapper.SetUserRedirect(_instr->result, AllocateSVOStructSequential(tokenMetadataStruct, metadataMap.Data(), kMetadataDWordCount));
        return;
    }

    // Allocate ids
    uint32_t rootOffset = program.GetIdentifierMap().AllocID();

    // All cbuffer rows, not all elements may be used
    TrivialStackVector<uint32_t, (kMetadataDWordCount + 3) / 4> legacyRows;

    // Get the current root offset for the descriptor, entirely scalarized
    if (userMapping.source)
    {
        // The row offset of the root parameter
        const uint32_t rowOffset = userMapping.source->dwordOffset / 4u;

        // Number of rows needed, if part of an indirection, just one element
        uint32_t rowCount = 1u;

        // If this is an inline root parameter, the metadata is packed in the root parameters
        if (userMapping.source->isRootResourceParameter) {
            // Determine the number of rows needed
            // Since the base dword offset may not be 0 for a given row, account for the intra row offset too
            const uint32_t texelOffset = userMapping.source->dwordOffset % 4u;
            rowCount = (kMetadataDWordCount + texelOffset + 3) / 4;
        }

        // Load all rows
        for (uint32_t i = 0; i < rowCount; i++) {
            // Allocate
            uint32_t legacyLoad = program.GetIdentifierMap().AllocID();
    
            // Get intrinsic
            const DXILFunctionDeclaration *intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpCBufferLoadLegacyI32);

            /*
              *  ; overloads: SM5.1: f32|i32|f64,  future SM: possibly deprecated
              *    %dx.types.CBufRet.f32 = type { float, float, float, float }
              *    declare %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(
              *       i32,                  ; opcode
              *       %dx.types.Handle,     ; resource handle
              *       i32)                  ; 0-based row index (row = 16-byte DXBC register)
              */

            uint64_t ops[3];

            ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                Backend::IL::IntConstant{.value = static_cast<uint32_t>(DXILOpcodes::CBufferLoadLegacy)}
            )->id);

            ops[1] = table.idRemapper.EncodeRedirectedUserOperand(descriptorHandle);

            ops[2] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                Backend::IL::IntConstant{.value = static_cast<int64_t>(rowOffset + i)}
            )->id);

            // Invoke
            block->AddRecord(CompileIntrinsicCall(legacyLoad, intrinsic, 3, ops));
            legacyRows.Add(legacyLoad);
        }
    }

    // Number of resource bindings
    IL::ID resourceVirtualBound = reservedConstantRange[static_cast<uint32_t>(ReservedConstantDataDWords::ResourceHeapInvalidationBound)];

    // Invalid binding identifier
    IL::ID invalidBindingId{ IL::InvalidID };
    
    // Root parameters are hosted inline
    if (userMapping.source && userMapping.source->isRootResourceParameter) {
        ASSERT(userMapping.dynamicOffset == IL::InvalidID, "Dynamic offset on inline root parameter");

        // Offset within the row
        uint32_t dwordOffset = userMapping.source->dwordOffset % 4u;

        // Extract respective value (uint4)
        for (uint32_t i = 0; i < kMetadataDWordCount; i++) {
            uint32_t fieldId = program.GetIdentifierMap().AllocID();
            
            LLVMRecord recordExtract(LLVMFunctionRecord::InstExtractVal);
            recordExtract.SetUser(true, ~0u, fieldId);
            recordExtract.opCount = 2;
            recordExtract.ops = table.recordAllocator.AllocateArray<uint64_t>(2);
            recordExtract.ops[0] = DXILIDRemapper::EncodeUserOperand(legacyRows[dwordOffset / 4]);
            recordExtract.ops[1] = dwordOffset % 4u;
            block->AddRecord(recordExtract);
            metadataMap.Add(fieldId);

            // Next dword
            dwordOffset++;
        }

        // Always the first one
        rootOffset = metadataMap[0];

        // Resource invalidation literals are tied to the heap bounds
        invalidBindingId = resourceVirtualBound;
    } else {
        // Determine the appropriate PRMT handle
        IL::ID prmtBufferId;
        switch (program.GetTypeMap().GetType(_instr->resource)->kind) {
            default:
                ASSERT(false, "Invalid resource type to get token from");
                return;
            case Backend::IL::TypeKind::CBuffer:
            case Backend::IL::TypeKind::Texture:
            case Backend::IL::TypeKind::Buffer: {
                prmtBufferId = resourcePRMTHandle;

                // Resource invalidation literals are tied to the heap bounds
                invalidBindingId = resourceVirtualBound;
                break;
            }
            case Backend::IL::TypeKind::Sampler: {
                prmtBufferId = samplerPRMTHandle;

                // Sampler invalidation literals are constant
                invalidBindingId = program.GetConstants().FindConstantOrAdd(
                    program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                    IL::IntConstant{.value = kDescriptorDataSamplerInvalidOffset}
                )->id;
                break;
            }
        }

        // Alloc IDs
        uint32_t descriptorOffset = program.GetIdentifierMap().AllocID();

        // Root wise offset?
        if (userMapping.source) {
            // Extract respective value (uint4)
            {
                // This is not an inline element, expecting just one row
                ASSERT(legacyRows.Size() == 1u, "Unexpected state");
                
                LLVMRecord recordExtract(LLVMFunctionRecord::InstExtractVal);
                recordExtract.SetUser(true, ~0u, rootOffset);
                recordExtract.opCount = 2;
                recordExtract.ops = table.recordAllocator.AllocateArray<uint64_t>(2);
                recordExtract.ops[0] = DXILIDRemapper::EncodeUserOperand(legacyRows[0]);
                recordExtract.ops[1] = userMapping.source->dwordOffset % 4u;
                block->AddRecord(recordExtract);
            }
        
            // Add local descriptor offset
            {
                LLVMRecord addRecord;
                addRecord.SetUser(true, ~0u, descriptorOffset);
                addRecord.id = static_cast<uint32_t>(LLVMFunctionRecord::InstBinOp);
                addRecord.opCount = 3u;
                addRecord.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                addRecord.ops[2] = static_cast<uint64_t>(LLVMBinOp::Add);

                addRecord.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(rootOffset);

                addRecord.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                    program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                    Backend::IL::IntConstant{.value = userMapping.source->offset}
                )->id);

                block->AddRecord(addRecord);
            }
        } else {
            // Source wise offset is the heap prefix
            descriptorOffset = program.GetConstants().FindConstantOrAdd(
                program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                Backend::IL::IntConstant{.value = 0u}
            )->id;
        }

        // Optional, out of bounds checking
        IL::ID outOfHeapOperand = IL::InvalidID;

        // Apply dynamic offset if valid
        if (userMapping.dynamicOffset != IL::InvalidID) {
            uint32_t extendedDescriptorOffset = program.GetIdentifierMap().AllocID();

            // CBOffset + DynamicOffset
            {
                LLVMRecord addRecord;
                addRecord.SetUser(true, ~0u, extendedDescriptorOffset);
                addRecord.id = static_cast<uint32_t>(LLVMFunctionRecord::InstBinOp);
                addRecord.opCount = 3u;
                addRecord.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                addRecord.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(descriptorOffset);
                addRecord.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(userMapping.dynamicOffset);
                addRecord.ops[2] = static_cast<uint64_t>(LLVMBinOp::Add);
                block->AddRecord(addRecord);
            }

            // Set new offset
            descriptorOffset = extendedDescriptorOffset;

            // Allocate out of bounds identifier
            outOfHeapOperand = program.GetIdentifierMap().AllocID();

            // DescriptorOffset > VRMTCount
            {
                LLVMRecord cmpRecord;
                cmpRecord.SetUser(true, ~0u, outOfHeapOperand);
                cmpRecord.id = static_cast<uint32_t>(LLVMFunctionRecord::InstCmp);
                cmpRecord.opCount = 3u;
                cmpRecord.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                cmpRecord.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(descriptorOffset);
                cmpRecord.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(resourceVirtualBound);
                cmpRecord.ops[2] = static_cast<uint64_t>(LLVMCmpOp::IntUnsignedGreaterEqual);
                block->AddRecord(cmpRecord);
            }
        }

        // Offset * MetadataStride
        uint32_t metadataOffset = program.GetIdentifierMap().AllocID();
        {
            LLVMRecord addRecord;
            addRecord.SetUser(true, ~0u, metadataOffset);
            addRecord.id = static_cast<uint32_t>(LLVMFunctionRecord::InstBinOp);
            addRecord.opCount = 3u;
            addRecord.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
            addRecord.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(descriptorOffset);
            addRecord.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().UInt(static_cast<uint32_t>(Backend::IL::ResourceTokenMetadataField::Count))->id);
            addRecord.ops[2] = static_cast<uint64_t>(LLVMBinOp::Mul);
            block->AddRecord(addRecord);
        }

        // Current offset
        uint32_t texelOffsetId = metadataOffset;

        // Load all dwords
        for (uint32_t i = 0; i < kMetadataDWordCount; i++) {
            uint32_t fieldLoadId = program.GetIdentifierMap().AllocID();
            uint32_t fieldExtractId = program.GetIdentifierMap().AllocID();

            // Advance each succeeding iteration by 1
            if (i != 0) {
                uint32_t nextTexelOffset = program.GetIdentifierMap().AllocID();

                // texelOffsetId + 1
                LLVMRecord addRecord(static_cast<uint32_t>(LLVMFunctionRecord::InstBinOp));
                addRecord.SetUser(true, ~0u, nextTexelOffset);
                addRecord.opCount = 3u;
                addRecord.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
                addRecord.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(texelOffsetId);
                addRecord.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                    program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                    Backend::IL::IntConstant{.value = 1}
                )->id);
                addRecord.ops[2] = static_cast<uint64_t>(LLVMBinOp::Add);
                block->AddRecord(addRecord);

                // Set as next
                texelOffsetId = nextTexelOffset;
            }
            
            // Load the resource token
            {
                // Get intrinsic
                const DXILFunctionDeclaration *intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpBufferLoadI32);

                /*
                 * ; overloads: SM5.1: f32|i32,  SM6.0: f32|i32
                 * ; returns: status
                 * declare %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(
                 *     i32,                  ; opcode
                 *     %dx.types.Handle,     ; resource handle
                 *     i32,                  ; coordinate c0
                 *     i32)                  ; coordinate c1
                 */

                uint64_t ops[4];

                ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                    program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                    Backend::IL::IntConstant{.value = static_cast<uint32_t>(DXILOpcodes::BufferLoad)}
                )->id);
                ops[1] = table.idRemapper.EncodeRedirectedUserOperand(prmtBufferId);
                ops[2] = table.idRemapper.EncodeRedirectedUserOperand(texelOffsetId);
                ops[3] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                    program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                    Backend::IL::UndefConstant{}
                )->id);

                // Invoke
                block->AddRecord(CompileIntrinsicCall(fieldLoadId, intrinsic, 4, ops));
            }
            
            // Extract first value
            LLVMRecord recordExtract(LLVMFunctionRecord::InstExtractVal);
            recordExtract.SetUser(true, ~0u, fieldExtractId);
            recordExtract.opCount = 2;
            recordExtract.ops = table.recordAllocator.AllocateArray<uint64_t>(2);
            recordExtract.ops[0] = DXILIDRemapper::EncodeUserOperand(fieldLoadId);
            recordExtract.ops[1] = 0;
            block->AddRecord(recordExtract);
            metadataMap.Add(fieldExtractId);
        }

        // Requires out of bounds safe-guarding?
        if (outOfHeapOperand != IL::InvalidID) {
            // Intermediate identifiers
            uint32_t oobValidated = program.GetIdentifierMap().AllocID();

            // OutOfBounds ? kResourceTokenPUIDInvalidOutOfBounds : ResourceToken
            LLVMRecord recordSelect(LLVMFunctionRecord::InstVSelect);
            recordSelect.SetUser(true, ~0u, oobValidated);
            recordSelect.opCount = 3;
            recordSelect.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
            recordSelect.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                Backend::IL::IntConstant{.value = IL::kResourceTokenPUIDInvalidOutOfBounds}
            )->id);
            recordSelect.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(metadataMap[0]);
            recordSelect.ops[2] = table.idRemapper.EncodeRedirectedUserOperand(outOfHeapOperand);
            block->AddRecord(recordSelect);
            metadataMap[0] = oobValidated;
        }
    }

    // Validate the root binding itself
    if (userMapping.source) {
        uint32_t isTableNotBound = program.GetIdentifierMap().AllocID();
        uint32_t selectValidated = program.GetIdentifierMap().AllocID();
        
        // CBufferData == Invalid
        LLVMRecord recordCmp(LLVMFunctionRecord::InstCmp);
        recordCmp.SetUser(true, ~0u, isTableNotBound);
        recordCmp.opCount = 3;
        recordCmp.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
        recordCmp.ops[0] = DXILIDRemapper::EncodeUserOperand(rootOffset);
        recordCmp.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(invalidBindingId);
        recordCmp.ops[2] = static_cast<uint64_t>(LLVMCmpOp::IntEqual);
        block->AddRecord(recordCmp);

        // TableNotBound ? kResourceTokenPUIDInvalidTableNotBound : ResourceToken
        LLVMRecord recordSelect(LLVMFunctionRecord::InstVSelect);
        recordSelect.SetUser(true, ~0u, selectValidated);
        recordSelect.opCount = 3;
        recordSelect.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
        recordSelect.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::IntConstant{.value = IL::kResourceTokenPUIDInvalidTableNotBound}
        )->id);
        recordSelect.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(metadataMap[0]);
        recordSelect.ops[2] = table.idRemapper.EncodeRedirectedUserOperand(isTableNotBound);
        block->AddRecord(recordSelect);
        metadataMap[0] = selectValidated;
    }

    // Allocate struct
    table.idRemapper.SetUserRedirect(_instr->result, AllocateSVOStructSequential(tokenMetadataStruct, metadataMap.Data(), kMetadataDWordCount));
}

void DXILPhysicalBlockFunction::CompileExportInstruction(LLVMBlock *block, const IL::ExportInstruction *_instr) {
    // Atomically incremented head index
    uint32_t atomicHead = program.GetIdentifierMap().AllocID();

    // Allocate the message
    {
        // Get intrinsic
        const DXILFunctionDeclaration *intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpAtomicBinOpI32);

        /*
         * ; overloads: SM5.1: i32,  SM6.0: i32
         * ; returns: original value in memory before the operation
         * declare i32 @dx.op.atomicBinOp.i32(
         *     i32,                  ; opcode
         *     %dx.types.Handle,     ; resource handle
         *     i32,                  ; binary operation code: EXCHANGE, IADD, AND, OR, XOR, IMIN, IMAX, UMIN, UMAX
         *     i32,                  ; coordinate c0
         *     i32,                  ; coordinate c1
         *     i32,                  ; coordinate c2
         *     i32)                  ; new value
         */

        uint64_t ops[7];

        ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::IntConstant{.value = static_cast<uint32_t>(DXILOpcodes::AtomicBinOp)}
        )->id);

        ops[1] = table.idRemapper.EncodeRedirectedUserOperand(exportCounterHandle);

        ops[2] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::IntConstant{.value = static_cast<uint32_t>(0)}
        )->id);

        ops[3] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::IntConstant{.value = _instr->exportID}
        )->id);

        ops[4] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::UndefConstant{}
        )->id);

        ops[5] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::UndefConstant{}
        )->id);

        ops[6] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::IntConstant{.value = static_cast<uint32_t>(_instr->values.count)}
        )->id);

        // Invoke
        block->AddRecord(CompileIntrinsicCall(atomicHead, intrinsic, 7, ops));
    }

    // Store the message
    for (uint32_t i = 0; i < _instr->values.count; i++)
    {
        // Get intrinsic
        const DXILFunctionDeclaration *intrinsic = table.intrinsics.GetIntrinsic(Intrinsics::DxOpBufferStoreI32);

        // Head offset for the current value
        IL::ID valueHead = atomicHead;

        // Increment if needed
        if (i > 0) {
            valueHead = program.GetIdentifierMap().AllocID();

            // AtomicHead + i
            LLVMRecord addRecord(static_cast<uint32_t>(LLVMFunctionRecord::InstBinOp));
            addRecord.SetUser(true, ~0u, valueHead);
            addRecord.opCount = 3u;
            addRecord.ops = table.recordAllocator.AllocateArray<uint64_t>(3);
            addRecord.ops[0] = table.idRemapper.EncodeRedirectedUserOperand(atomicHead);
            addRecord.ops[1] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
                program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
                Backend::IL::IntConstant{.value = i}
            )->id);
            addRecord.ops[2] = static_cast<uint64_t>(LLVMBinOp::Add);
            block->AddRecord(addRecord);
        }

        /*
         * ; overloads: SM5.1: f32|i32,  SM6.0: f32|i32
         * declare void @dx.op.bufferStore.i32(
         *     i32,                  ; opcode
         *     %dx.types.Handle,     ; resource handle
         *     i32,                  ; coordinate c0
         *     i32,                  ; coordinate c1
         *     i32,                  ; value v0
         *     i32,                  ; value v1
         *     i32,                  ; value v2
         *     i32,                  ; value v3
         *     i8)                   ; write mask
         */

        uint64_t ops[9];

        ops[0] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::IntConstant{.value = static_cast<uint32_t>(DXILOpcodes::BufferStore)}
        )->id);

        ops[1] = table.idRemapper.EncodeRedirectedUserOperand(exportStreamHandles[_instr->exportID]);

        ops[2] = table.idRemapper.EncodeRedirectedUserOperand(valueHead);

        ops[3] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=32, .signedness=true}),
            Backend::IL::UndefConstant{}
        )->id);

        ops[4] = table.idRemapper.EncodeRedirectedUserOperand(_instr->values[i]);
        ops[5] = table.idRemapper.EncodeRedirectedUserOperand(_instr->values[i]);
        ops[6] = table.idRemapper.EncodeRedirectedUserOperand(_instr->values[i]);
        ops[7] = table.idRemapper.EncodeRedirectedUserOperand(_instr->values[i]);

        ops[8] = table.idRemapper.EncodeRedirectedUserOperand(program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth=8, .signedness=true}),
            Backend::IL::IntConstant{.value = static_cast<uint32_t>(IL::ComponentMask::All)}
        )->id);

        // Invoke
        block->AddRecord(CompileIntrinsicCall(IL::InvalidID, intrinsic, 9, ops));
    }
}
