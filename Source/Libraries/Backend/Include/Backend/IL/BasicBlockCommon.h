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

        // Keep track of all redirects
        idMap.RedirectInstruction(instr->result, result);

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

    /// Find the first non phi instruction
    /// \param block given block
    /// \param it iterator to search from
    /// \return first non phi, may be end
    inline BasicBlock::Iterator FirstNonPhi(BasicBlock* block, BasicBlock::Iterator it) {
        while (it != block->end() && it->Is<PhiInstruction>()) {
            ++it;
        }

        return it;
    }
}
