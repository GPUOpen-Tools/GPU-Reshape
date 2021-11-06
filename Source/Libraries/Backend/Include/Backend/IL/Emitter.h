#pragma once

// Backend
#include "BasicBlock.h"
#include "Program.h"

namespace IL {
    struct Emitter {
        /// Set the current program
        void SetProgram(Program* value) {
            program = value;
        }

        /// Set the current basic block
        void SetBasicBlock(BasicBlock* value) {
            basicBlock = value;
        }

        /// Emit an unexposed instruction
        /// \param result optional, the result id
        /// \param backendOpCode the unexposed opcode value
        void Unexposed(ID result, uint32_t backendOpCode) {
            UnexposedInstruction instr{};
            instr.opCode = OpCode::Unexposed;
            instr.result = result;
            basicBlock->Insert(instr);
        }

        /// Load an address
        /// \param address the address to be loaded
        /// \return the result id
        ID Load(ID address) {
            LoadInstruction instr{};
            instr.opCode = OpCode::Load;
            instr.address = address;
            instr.result = program->AllocID();
            basicBlock->Insert(instr);

            return instr.result;
        }

        /// Store to an address
        /// \param address the address to be stored to
        /// \param value the value to be stored
        void Store(ID address, ID value) {
            StoreInstruction instr{};
            instr.opCode = OpCode::Store;
            instr.address = address;
            instr.value = value;
            basicBlock->Insert(instr);
        }

        /// Is this emitter good?
        bool Good() const {
            return program && basicBlock;
        }

        /// Get the current program
        Program* GetProgram() const {
            return program;
        }

        /// Get the current basic block
        BasicBlock* GetBasicBlock() const {
            return basicBlock;
        }

    private:
        /// Current program
        Program* program{nullptr};

        /// Current basic block
        BasicBlock* basicBlock{nullptr};
    };
}
