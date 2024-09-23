#pragma once

// Backend
#include <Backend/IL/InstructionCommon.h>

namespace IL {
    bool CanFoldWithImmediates(const Instruction* instr) {
        switch (instr->opCode) {
            default: {
                return true;
            }

            /** Custom instructions can never be folded */
            case OpCode::Export:
            case OpCode::ResourceSize:
            case OpCode::ResourceToken:

            /** Inter-stages can never be folded */
            case OpCode::StoreOutput:
            case OpCode::StoreVertexOutput:
            case OpCode::StorePrimitiveOutput:

            /** No resource immediates */
            case OpCode::SampleTexture:
            case OpCode::StoreTexture:
            case OpCode::LoadTexture:
            case OpCode::StoreBuffer:
            case OpCode::LoadBuffer:
            case OpCode::StoreBufferRaw:
            case OpCode::LoadBufferRaw:

            /** No CFG immediates */
            case OpCode::Branch:
            case OpCode::BranchConditional:
            case OpCode::Return:
            case OpCode::Switch:
            case OpCode::Phi:
            case OpCode::Select:

            /** Interprocedurals are not "immediate" folds, I wish... */
            case OpCode::Call:

            /** Memory operations cannot be folded with immediates */
            case OpCode::Load:
            case OpCode::Store:

            /** No atomic immediates */
            case OpCode::AtomicOr:
            case OpCode::AtomicXOr:
            case OpCode::AtomicAnd:
            case OpCode::AtomicAdd:
            case OpCode::AtomicMin:
            case OpCode::AtomicMax:
            case OpCode::AtomicExchange: {
                return false;
            }

            /** Assume unexposed by backend */
            case OpCode::Unexposed: {
                auto* _instr = instr->As<UnexposedInstruction>();
                return _instr->traits.foldableWithImmediates;
            }
        }
    }
}
