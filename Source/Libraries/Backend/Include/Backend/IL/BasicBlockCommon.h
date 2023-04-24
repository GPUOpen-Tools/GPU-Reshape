#pragma once

// Backend
#include "Program.h"

namespace Backend::IL {
    /// Redirect an instruction result
    /// \param program source program
    /// \param it source instruction
    /// \param result new result id
    inline void RedirectResult(Program& program, const OpaqueInstructionRef& it, ID result) {
        Instruction* instr = it.basicBlock->GetRelocationInstruction(it.relocationOffset);

        // Remap the identifier map
        IdentifierMap& idMap = program.GetIdentifierMap();
        idMap.RemoveInstruction(instr->result);
        idMap.AddInstruction(it, result);

        // Remap the type map
        TypeMap& typeMap = program.GetTypeMap();
        typeMap.SetType(result, typeMap.GetType(instr->result));
        typeMap.RemoveType(instr->result);

        // Change result
        instr->result = result;

        // Mark user instruction as dirty
        instr->source = instr->source.Modify();

        // Ensure the basic block is recompiled
        it.basicBlock->MarkAsDirty();
    }
}
