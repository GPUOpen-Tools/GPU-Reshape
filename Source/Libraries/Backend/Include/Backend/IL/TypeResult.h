#pragma once

#include "Program.h"
#include "Instruction.h"

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

    inline const Type* ResultOf(Program& program, const AndInstruction* instr) {
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
    }

    inline const Type* ResultOf(Program& program, const OrInstruction* instr) {
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
    }

    inline const Type* ResultOf(Program& program, const EqualInstruction* instr) {
        const Type* type = program.GetTypeMap().GetType(instr->lhs);
        return ReplaceVectorizedType(program, type, program.GetTypeMap().FindTypeOrAdd(BoolType{}));
    }

    inline const Type* ResultOf(Program& program, const NotEqualInstruction* instr) {
        const Type* type = program.GetTypeMap().GetType(instr->lhs);
        return ReplaceVectorizedType(program, type, program.GetTypeMap().FindTypeOrAdd(BoolType{}));
    }

    inline const Type* ResultOf(Program& program, const GreaterThanInstruction* instr) {
        const Type* type = program.GetTypeMap().GetType(instr->lhs);
        return ReplaceVectorizedType(program, type, program.GetTypeMap().FindTypeOrAdd(BoolType{}));
    }

    inline const Type* ResultOf(Program& program, const GreaterThanEqualInstruction* instr) {
        const Type* type = program.GetTypeMap().GetType(instr->lhs);
        return ReplaceVectorizedType(program, type, program.GetTypeMap().FindTypeOrAdd(BoolType{}));
    }

    inline const Type* ResultOf(Program& program, const LessThanInstruction* instr) {
        const Type* type = program.GetTypeMap().GetType(instr->lhs);
        return ReplaceVectorizedType(program, type, program.GetTypeMap().FindTypeOrAdd(BoolType{}));
    }

    inline const Type* ResultOf(Program& program, const LessThanEqualInstruction* instr) {
        const Type* type = program.GetTypeMap().GetType(instr->lhs);
        return ReplaceVectorizedType(program, type, program.GetTypeMap().FindTypeOrAdd(BoolType{}));
    }

    inline const Type* ResultOf(Program& program, const LoadTextureInstruction* instr) {
        const Type* texture = program.GetTypeMap().GetType(instr->texture);
        return texture->As<TextureType>()->sampledType;
    }

    inline const Type* ResultOf(Program& program, const LoadBufferInstruction* instr) {
        const Type* buffer = program.GetTypeMap().GetType(instr->buffer);
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

    inline const Type* ResultOf(Program& program, const ResourceSizeInstruction* instr) {
        const Type* resource = program.GetTypeMap().GetType(instr->resource);

        switch (resource->kind) {
            default:
                ASSERT(false, "Invalid ResourceSize instruction");
                return nullptr;
            case TypeKind::Texture:
                return program.GetTypeMap().FindTypeOrAdd(VectorType {
                    .containedType = program.GetTypeMap().FindTypeOrAdd(IntType { .bitWidth = 32, .signedness = false }),
                    .dimension = static_cast<uint8_t>(GetDimensionSize(resource->As<TextureType>()->dimension))
                });
            case TypeKind::Buffer:
                return program.GetTypeMap().FindTypeOrAdd(IntType { .bitWidth = 32, .signedness = false });
        }
    }
}
