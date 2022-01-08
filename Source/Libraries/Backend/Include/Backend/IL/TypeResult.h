#pragma once

#include "Program.h"
#include "Instruction.h"

namespace Backend::IL {
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
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
    }

    inline const Type* ResultOf(Program& program, const NotEqualInstruction* instr) {
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
    }

    inline const Type* ResultOf(Program& program, const GreaterThanInstruction* instr) {
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
    }

    inline const Type* ResultOf(Program& program, const GreaterThanEqualInstruction* instr) {
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
    }

    inline const Type* ResultOf(Program& program, const LessThanInstruction* instr) {
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
    }

    inline const Type* ResultOf(Program& program, const LessThanEqualInstruction* instr) {
        return program.GetTypeMap().FindTypeOrAdd(BoolType{});
    }

    inline const Type* ResultOf(Program& program, const LoadTextureInstruction* instr) {
        const Type* texture = program.GetTypeMap().GetType(instr->texture);
        return texture->As<TextureType>()->sampledType;
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
}
