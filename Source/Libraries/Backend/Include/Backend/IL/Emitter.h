#pragma once

// Backend
#include "BasicBlock.h"
#include "Program.h"

namespace IL {
    struct Emitter {
        explicit Emitter(Program& program, BasicBlock& basicBlock) : program(program), basicBlock(basicBlock) {

        }

        ID Load(ID address) {
            LoadInstruction instr{};
            instr.opCode = OpCode::Load;
            instr.address = address;
            instr.result = program.AllocID();
            basicBlock.Insert(instr);

            return instr.result;
        }

        void Store(ID address, ID value) {
            StoreInstruction instr{};
            instr.opCode = OpCode::Store;
            instr.address = address;
            instr.value = value;
            basicBlock.Insert(instr);
        }

    private:
        Program& program;
        BasicBlock& basicBlock;
    };
}
