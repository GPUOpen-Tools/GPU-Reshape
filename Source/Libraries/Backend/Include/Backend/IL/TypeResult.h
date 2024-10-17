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

#pragma once

#include "Program.h"
#include "Instruction.h"
#include "TypeCommon.h"

namespace Backend::IL {
    inline const Type* ReplaceVectorizedType(Program& program, const Type* type, const Type* component) {
        switch (type->kind) {
            default:
                return component;
            case TypeKind::Vector: {
                auto* vector = type->As<VectorType>();
                return program.GetTypeMap().FindTypeOrAdd(VectorType{
                    .containedType = component,
                    .dimension = vector->dimension
                });
            }
            case TypeKind::Matrix: {
                auto* vector = type->As<MatrixType>();
                return program.GetTypeMap().FindTypeOrAdd(MatrixType{
                    .containedType = component,
                    .rows = vector->rows,
                    .columns = vector->columns
                });
            }
        }
    }

    inline const Type* ResultOf(Program& program, const Instruction* instr) {
        return nullptr;
    }

    inline const Type* ResultOf(Program& program, const LoadInstruction* instr) {
        const Type* addr = program.GetTypeMap().GetType(instr->address);
        if (!addr) {
            ASSERT(false, "Failed to determine type");
            return nullptr;
        }

        return addr->As<PointerType>()->pointee;
    }

    inline const Type* ResultOf(Program& program, const RemInstruction* instr) {
        return program.GetTypeMap().GetType(instr->lhs);
    }

    inline const Type* ResultOf(Program& program, const AddInstruction* instr) {
        return program.GetTypeMap().GetType(instr->lhs);
    }

    inline const Type* ResultOf(Program& program, const SubInstruction* instr) {
        return program.GetTypeMap().GetType(instr->lhs);
    }

    inline const Type* ResultOf(Program& program, const DivInstruction* instr) {
        return program.GetTypeMap().GetType(instr->lhs);
    }

    inline const Type* ResultOf(Program& program, const MulInstruction* instr) {
        return program.GetTypeMap().GetType(instr->lhs);
    }

    inline const Type* ResultOf(Program& program, const BitAndInstruction* instr) {
        return program.GetTypeMap().GetType(instr->lhs);
    }

    inline const Type* ResultOf(Program& program, const BitOrInstruction* instr) {
        return program.GetTypeMap().GetType(instr->lhs);
    }

    inline const Type* ResultOf(Program& program, const BitShiftLeftInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const BitShiftRightInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const SelectInstruction* instr) {
        return program.GetTypeMap().GetType(instr->pass);
    }

    inline const Type* ResultOf(Program& program, const PhiInstruction* instr) {
        return program.GetTypeMap().GetType(instr->values[0].value);
    }

    inline const Type* ResultOf(Program& program, const NotInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const AndInstruction* instr) {
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
    }

    inline const Type* ResultOf(Program& program, const OrInstruction* instr) {
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
    }

    inline const Type* ResultOf(Program& program, const AtomicAndInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const AtomicOrInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const AtomicXOrInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const AtomicAddInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const AtomicMinInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const AtomicMaxInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const AtomicExchangeInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const AtomicCompareExchangeInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const WaveAnyTrueInstruction* instr) {
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
    }

    inline const Type* ResultOf(Program& program, const WaveAllTrueInstruction* instr) {
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
    }

    inline const Type* ResultOf(Program& program, const WaveBallotInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const WaveReadInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const WaveReadFirstInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const WaveAllEqualInstruction* instr) {
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
    }

    inline const Type* ResultOf(Program& program, const WaveBitAndInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const WaveBitOrInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const WaveBitXOrInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const WaveCountBitsInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const WaveMaxInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const WaveMinInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const WaveProductInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const WaveSumInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const WavePrefixCountBitsInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const WavePrefixProductInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const WavePrefixSumInstruction* instr) {
        return program.GetTypeMap().GetType(instr->value);
    }

    inline const Type* ResultOf(Program& program, const IsNaNInstruction* instr) {
        return SplatToValue(program, program.GetTypeMap().FindTypeOrAdd(BoolType{}), instr->value);
    }

    inline const Type* ResultOf(Program& program, const KernelValueInstruction* instr) {
        switch (instr->value) {
            default: {
                ASSERT(false, "Invalid value");
                return nullptr;
            }
            case KernelValue::DispatchThreadID: {
                return program.GetTypeMap().FindTypeOrAdd(VectorType{
                    .containedType = program.GetTypeMap().FindTypeOrAdd(IntType { .bitWidth = 32, .signedness = false }),
                    .dimension = 3
                });
            }
            case KernelValue::FlattenedLocalThreadID: {
                return program.GetTypeMap().FindTypeOrAdd(IntType { .bitWidth = 32, .signedness = false });
            }
        }
    }

    inline const Type* ResultOf(Program& program, const ExtendedInstruction* instr) {
        switch (instr->extendedOp) {
            default:
                ASSERT(false, "Invalid extended op");
                return nullptr;
            case ExtendedOp::Min:
            case ExtendedOp::Max:
            case ExtendedOp::Abs:
            case ExtendedOp::Floor:
            case ExtendedOp::Ceil:
            case ExtendedOp::Round:
            case ExtendedOp::Pow:
            case ExtendedOp::Exp:
            case ExtendedOp::Sqrt:
            case ExtendedOp::FirstBitLow:
            case ExtendedOp::FirstBitHigh:
                return program.GetTypeMap().GetType(instr->operands[0]);
        }
    }

    inline const Type* ResultOf(Program& program, const IsInfInstruction* instr) {
        return SplatToValue(program, program.GetTypeMap().FindTypeOrAdd(BoolType{}), instr->value);
    }

    inline const Type* ResultOf(Program& program, const EqualInstruction* instr) {
        const Type* type = program.GetTypeMap().GetType(instr->lhs);
        if (!type) {
            ASSERT(false, "Failed to determine type");
            return nullptr;
        }

        return ReplaceVectorizedType(program, type, program.GetTypeMap().FindTypeOrAdd(BoolType{}));
    }

    inline const Type* ResultOf(Program& program, const NotEqualInstruction* instr) {
        const Type* type = program.GetTypeMap().GetType(instr->lhs);
        if (!type) {
            ASSERT(false, "Failed to determine type");
            return nullptr;
        }

        return ReplaceVectorizedType(program, type, program.GetTypeMap().FindTypeOrAdd(BoolType{}));
    }

    inline const Type* ResultOf(Program& program, const GreaterThanInstruction* instr) {
        const Type* type = program.GetTypeMap().GetType(instr->lhs);
        if (!type) {
            ASSERT(false, "Failed to determine type");
            return nullptr;
        }

        return ReplaceVectorizedType(program, type, program.GetTypeMap().FindTypeOrAdd(BoolType{}));
    }

    inline const Type* ResultOf(Program& program, const GreaterThanEqualInstruction* instr) {
        const Type* type = program.GetTypeMap().GetType(instr->lhs);
        if (!type) {
            ASSERT(false, "Failed to determine type");
            return nullptr;
        }

        return ReplaceVectorizedType(program, type, program.GetTypeMap().FindTypeOrAdd(BoolType{}));
    }

    inline const Type* ResultOf(Program& program, const LessThanInstruction* instr) {
        const Type* type = program.GetTypeMap().GetType(instr->lhs);
        if (!type) {
            ASSERT(false, "Failed to determine type");
            return nullptr;
        }

        return ReplaceVectorizedType(program, type, program.GetTypeMap().FindTypeOrAdd(BoolType{}));
    }

    inline const Type* ResultOf(Program& program, const LessThanEqualInstruction* instr) {
        const Type* type = program.GetTypeMap().GetType(instr->lhs);
        if (!type) {
            ASSERT(false, "Failed to determine type");
            return nullptr;
        }

        return ReplaceVectorizedType(program, type, program.GetTypeMap().FindTypeOrAdd(BoolType{}));
    }

    inline const Type* ResultOf(Program& program, const LoadTextureInstruction* instr) {
        const Type* texture = program.GetTypeMap().GetType(instr->texture);
        if (!texture) {
            ASSERT(false, "Failed to determine type");
            return nullptr;
        }

        return texture->As<TextureType>()->sampledType;
    }

    inline const Type* ResultOf(Program& program, const SampleTextureInstruction* instr) {
        const Type* texture = program.GetTypeMap().GetType(instr->texture);
        if (!texture) {
            ASSERT(false, "Failed to determine type");
            return nullptr;
        }

        return texture->As<TextureType>()->sampledType;
    }

    inline const Type* ResultOf(Program& program, const LoadBufferInstruction* instr) {
        const Type* buffer = program.GetTypeMap().GetType(instr->buffer);
        if (!buffer) {
            ASSERT(false, "Failed to determine type");
            return nullptr;
        }

        return Splat(program, buffer->As<BufferType>()->elementType, 4u);
    }

    inline const Type* ResultOf(Program& program, const LiteralInstruction* instr) {
        switch (instr->type) {
            default:
                ASSERT(false, "Invalid literal type");
                return nullptr;
            case IL::LiteralType::Int: {
                Backend::IL::IntType typeInt;
                typeInt.bitWidth = instr->bitWidth;
                typeInt.signedness = instr->signedness;
                return program.GetTypeMap().FindTypeOrAdd(typeInt);
            }
            case IL::LiteralType::FP: {
                Backend::IL::FPType typeFP;
                typeFP.bitWidth = instr->bitWidth;
                return program.GetTypeMap().FindTypeOrAdd(typeFP);
            }
        }
    }

    inline const Type* ResultOf(Program& program, const AnyInstruction* instr) {
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
    }

    inline const Type* ResultOf(Program& program, const AllInstruction* instr) {
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
    }

    inline const Type* ResultOf(Program& program, const ResourceTokenInstruction* instr) {
        return program.GetTypeMap().GetResourceToken();
    }

    inline const Type* ResultOf(Program& program, const ResourceSizeInstruction* instr) {
        const Type* resource = program.GetTypeMap().GetType(instr->resource);
        if (!resource) {
            ASSERT(false, "Failed to determine type");
            return nullptr;
        }

        switch (resource->kind) {
            default: {
                ASSERT(false, "Invalid ResourceSize instruction");
                return nullptr;
            }
            case TypeKind::Texture: {
                auto dimension = static_cast<uint8_t>(GetDimensionSize(resource->As<TextureType>()->dimension, true));
                if (dimension == 1) {
                    return program.GetTypeMap().FindTypeOrAdd(IntType { .bitWidth = 32, .signedness = false });
                }

                return program.GetTypeMap().FindTypeOrAdd(VectorType {
                    .containedType = program.GetTypeMap().FindTypeOrAdd(IntType { .bitWidth = 32, .signedness = false }),
                    .dimension = dimension
                });
            }
            case TypeKind::Buffer: {
                return program.GetTypeMap().FindTypeOrAdd(IntType { .bitWidth = 32, .signedness = false });
            }
        }
    }

    inline const Type* ResultOf(Program& program, const AddressChainInstruction* instr) {
        const Type* type = program.GetTypeMap().GetType(instr->composite);
        if (!type) {
            ASSERT(false, "Failed to determine type");
            return nullptr;
        }

        IL::AddressSpace space = IL::AddressSpace::Unexposed;

        // Walk the chain
        for (uint32_t i = 0; i < instr->chains.count; i++) {
            switch (type->kind) {
                default:
                ASSERT(false, "Unexpected GEP chain type");
                    break;
                case Backend::IL::TypeKind::Vector: {
                    type = type->As<Backend::IL::VectorType>()->containedType;
                    break;
                }
                case Backend::IL::TypeKind::Matrix: {
                    auto* matrix = type->As<Backend::IL::MatrixType>();
                    type = program.GetTypeMap().FindTypeOrAdd(Backend::IL::VectorType {
                        .containedType = matrix->containedType,
                        .dimension = matrix->rows
                    });
                    break;
                }
                case Backend::IL::TypeKind::Pointer:{
                    auto* pointer = type->As<Backend::IL::PointerType>();

                    // Texel pointers handled separately
                    switch (pointer->pointee->kind) {
                        default: {
                            ASSERT(space == AddressSpace::Unexposed || space == pointer->addressSpace, "Mismatched address space in address chain");
                            type = pointer->pointee;
                            space = pointer->addressSpace;
                        }
                        case Backend::IL::TypeKind::Buffer: {
                            type = pointer->pointee->As<Backend::IL::BufferType>()->elementType;
                            space = AddressSpace::Buffer;
                            break;
                        }
                        case Backend::IL::TypeKind::Texture: {
                            type = pointer->pointee->As<Backend::IL::TextureType>()->sampledType;
                            space = AddressSpace::Texture;
                            break;
                        }
                    }
                    break;
                }
                case Backend::IL::TypeKind::Array: {
                    type = type->As<Backend::IL::ArrayType>()->elementType;
                    break;
                }
                case Backend::IL::TypeKind::Struct: {
                    const Backend::IL::Constant* constant = program.GetConstants().GetConstant(instr->chains[i].index);
                    ASSERT(constant, "Struct chains must be constant");

                    auto memberIdx = static_cast<uint32_t>(constant->As<Backend::IL::IntConstant>()->value);
                    type = type->As<Backend::IL::StructType>()->memberTypes[memberIdx];
                    break;
                }
            }
        }

        ASSERT(space != IL::AddressSpace::Unexposed, "No AddressOf chain supplied a relevant address space, invalid");
        return program.GetTypeMap().FindTypeOrAdd(PointerType { .pointee = type, .addressSpace = space });
    }

    inline const Type* ResultOf(Program& program, const ExtractInstruction* instr) {
        const Type* type = program.GetTypeMap().GetType(instr->composite);
        if (!type) {
            ASSERT(false, "Failed to determine type");
            return nullptr;
        }

        // Walk the chain
        for (uint32_t i = 0; i < instr->chains.count; i++) {
            switch (type->kind) {
                default:
                    ASSERT(false, "Unexpected GEP chain type");
                    return nullptr;
                case Backend::IL::TypeKind::None:
                    return nullptr;
                case Backend::IL::TypeKind::Buffer:
                    type = type->As<Backend::IL::BufferType>()->elementType;
                    break;
                case Backend::IL::TypeKind::Texture:
                    type = type->As<Backend::IL::TextureType>()->sampledType;
                    break;
                case Backend::IL::TypeKind::Vector:
                    type = type->As<Backend::IL::VectorType>()->containedType;
                    break;
                case Backend::IL::TypeKind::Matrix:
                    type = type->As<Backend::IL::MatrixType>()->containedType;
                    break;
                case Backend::IL::TypeKind::Pointer:
                    type = type->As<Backend::IL::PointerType>()->pointee;
                    break;
                case Backend::IL::TypeKind::Array:
                    type = type->As<Backend::IL::ArrayType>()->elementType;
                    break;
                case Backend::IL::TypeKind::Struct: {
                    const Constant* index = program.GetConstants().GetConstant(instr->chains[i].index);
                    ASSERT(index, "Dynamic structured extraction not supported");
                    type = type->As<Backend::IL::StructType>()->memberTypes[index->As<IntConstant>()->value];
                }
            }
        }

        return type;
    }
}
