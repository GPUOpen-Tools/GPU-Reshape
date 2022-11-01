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

    inline const Type* ResultOf(Program& program, const AndInstruction* instr) {
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
    }

    inline const Type* ResultOf(Program& program, const OrInstruction* instr) {
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
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

    inline const Type* ResultOf(Program& program, const IsNaNInstruction* instr) {
        return SplatToValue(program, program.GetTypeMap().FindTypeOrAdd(BoolType{}), instr->value);
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

    inline const Type* ResultOf(Program& program, const LoadBufferInstruction* instr) {
        const Type* buffer = program.GetTypeMap().GetType(instr->buffer);
        if (!buffer) {
            ASSERT(false, "Failed to determine type");
            return nullptr;
        }

        return buffer->As<BufferType>()->elementType;
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
        return program.GetTypeMap().FindTypeOrAdd(IntType{ .bitWidth=32, .signedness=false });
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
                auto dimension = static_cast<uint8_t>(GetDimensionSize(resource->As<TextureType>()->dimension));
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

        // Walk the chain
        for (uint32_t i = 0; i < instr->chains.count; i++) {
            switch (type->kind) {
                default:
                ASSERT(false, "Unexpected GEP chain type");
                    break;
                case Backend::IL::TypeKind::None:
                    break;
                case Backend::IL::TypeKind::Buffer: {
                    type = type->As<Backend::IL::BufferType>()->elementType;
                    break;
                }
                case Backend::IL::TypeKind::Texture: {
                    type = type->As<Backend::IL::TextureType>()->sampledType;
                    break;
                }
                case Backend::IL::TypeKind::Vector: {
                    type = type->As<Backend::IL::VectorType>()->containedType;
                    break;
                }
                case Backend::IL::TypeKind::Matrix: {
                    type = type->As<Backend::IL::MatrixType>()->containedType;
                    break;
                }
                case Backend::IL::TypeKind::Pointer:{
                    type = type->As<Backend::IL::PointerType>()->pointee;
                    break;
                }
                case Backend::IL::TypeKind::Array:{
                    type = type->As<Backend::IL::ArrayType>()->elementType;
                    break;
                }
                case Backend::IL::TypeKind::Struct: {
                    const Backend::IL::Constant* constant = program.GetConstants().GetConstant(instr->chains[i].index);
                    ASSERT(constant, "Struct chains must be constant");

                    uint32_t memberIdx = constant->As<Backend::IL::IntConstant>()->value;
                    type = type->As<Backend::IL::StructType>()->memberTypes[memberIdx];
                    break;
                }
            }

            // TODO: Is function address space really right? Maybe derive from the source composite
            return program.GetTypeMap().FindTypeOrAdd(PointerType { .pointee = type, .addressSpace = AddressSpace::Function });
        }

        return type;
    }
}
