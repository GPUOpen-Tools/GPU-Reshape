#pragma once

// Backend
#include "Instruction.h"

namespace Backend::IL {
    /// Get the control flow of an instruction
    /// \param instr given instruction
    /// \param out output control flow
    /// \return true if the instruction has control flow
    bool GetControlFlow(const IL::Instruction* instr, IL::BranchControlFlow& out) {
        switch (instr->opCode) {
            default: {
                return false;
            }
            case IL::OpCode::Branch: {
                out = instr->As<IL::BranchInstruction>()->controlFlow;
                return true;
            }
            case IL::OpCode::BranchConditional: {
                out = instr->As<IL::BranchConditionalInstruction>()->controlFlow;
                return true;
            }
        }
    }
}
