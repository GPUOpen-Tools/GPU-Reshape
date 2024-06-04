#pragma once

// Backend
#include <Backend/IL/InstructionCommon.h>

namespace Backend::IL {
    bool CanFoldWithImmediates(const ::IL::Instruction* instr) {
        switch (instr->opCode) {
            default: {
                return true;
            }

            /** Custom instructions can never be folded */
            case ::IL::OpCode::Export:
            case ::IL::OpCode::ResourceSize:
            case ::IL::OpCode::ResourceToken:

            /** Inter-stages can never be folded */
            case ::IL::OpCode::StoreOutput:
            case ::IL::OpCode::StoreVertexOutput:
            case ::IL::OpCode::StorePrimitiveOutput:
            case ::IL::OpCode::EmitIndices:

            /** No resource immediates */
            case ::IL::OpCode::SampleTexture:
            case ::IL::OpCode::StoreTexture:
            case ::IL::OpCode::LoadTexture:
            case ::IL::OpCode::StoreBuffer:
            case ::IL::OpCode::LoadBuffer:

            /** No CFG immediates */
            case ::IL::OpCode::Branch:
            case ::IL::OpCode::BranchConditional:
            case ::IL::OpCode::Return:
            case ::IL::OpCode::Switch:
            case ::IL::OpCode::Phi:
            case ::IL::OpCode::Select:

            /** Interprocedurals are not "immediate" folds, I wish... */
            case ::IL::OpCode::Call:

            /** Memory operations cannot be folded with immediates */
            case ::IL::OpCode::Load:
            case ::IL::OpCode::Store:

            /** No atomic immediates */
            case ::IL::OpCode::AtomicOr:
            case ::IL::OpCode::AtomicXOr:
            case ::IL::OpCode::AtomicAnd:
            case ::IL::OpCode::AtomicAdd:
            case ::IL::OpCode::AtomicMin:
            case ::IL::OpCode::AtomicMax:
            case ::IL::OpCode::AtomicExchange: {
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
