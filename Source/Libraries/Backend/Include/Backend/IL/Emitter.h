#pragma once

// Backend
#include "BasicBlock.h"
#include "Program.h"

namespace IL {
    /// Append operation
    struct EmitterOpAppend {
        using Opaque = ConstOpaqueInstructionRef;

        /// Operation
        template<typename T>
        static InstructionRef <T> Op(BasicBlock *basicBlock, Opaque &insertionPoint, const T &instruction) {
            if (insertionPoint.IsValid()) {
                auto value = basicBlock->Insert(insertionPoint, instruction);
                insertionPoint = value;
                return value;
            } else {
                auto value = basicBlock->Insert(instruction);
                insertionPoint = value;
                return value;
            }
        }
    };

    /// Replacement operation
    struct EmitterOpReplace {
        using Opaque = OpaqueInstructionRef;

        /// Operation
        template<typename T>
        static InstructionRef <T> Op(BasicBlock *basicBlock, Opaque &insertionPoint, const T &instruction) {
            if (insertionPoint.IsValid()) {
                auto value = basicBlock->Replace(insertionPoint, instruction);
                insertionPoint = value;
                return value;
            } else {
                auto value = basicBlock->Insert(instruction);
                insertionPoint = value;
                return value;
            }
        }
    };

    /// Emitter, easy instruction emitting
    template<typename OP = EmitterOpAppend>
    struct Emitter {
        using Opaque = typename OP::Opaque;

        /// Default constructor
        Emitter() = default;

        /// Constructor
        Emitter(Program &program, BasicBlock &basicBlock, const Opaque &ref = {}) {
            SetProgram(&program);
            SetBasicBlock(&basicBlock);
            SetInsertionPoint(ref);
        }

        /// Set the current program
        void SetProgram(Program *value) {
            program = value;
            map = &value->GetIdentifierMap();
        }

        /// Set the current basic block
        void SetBasicBlock(BasicBlock *value) {
            basicBlock = value;
        }

        /// Set the insertion point
        void SetInsertionPoint(const Opaque &ref) {
            insertionPoint = ref;
        }

        /// Add an integral instruction
        /// \param result the result id
        /// \param bitWidth the bit width of the type
        /// \param value the constant value
        /// \return instruction reference
        InstructionRef <LiteralInstruction> Integral(ID result, uint8_t bitWidth, int64_t value) {
            LiteralInstruction instr{};
            instr.opCode = OpCode::Literal;
            instr.result = result;
            instr.type = LiteralType::Int;
            instr.bitWidth = bitWidth;
            instr.value.integral = value;
            return Op(instr);
        }

        /// Add an integral instruction
        /// \param bitWidth the bit width of the type
        /// \param value the constant value
        /// \return instruction reference
        InstructionRef <LiteralInstruction> Integral(uint8_t bitWidth, int64_t value) {
            return Integral(map->AllocID(), bitWidth, value);
        }

        /// Add a floating point instruction
        /// \param result the result id
        /// \param bitWidth the bit width of the type
        /// \param value the constant value
        /// \return instruction reference
        InstructionRef <LiteralInstruction> FP(ID result, uint8_t bitWidth, double value) {
            LiteralInstruction instr{};
            instr.opCode = OpCode::Literal;
            instr.result = result;
            instr.type = LiteralType::FP;
            instr.bitWidth = bitWidth;
            instr.value.fp = value;
            return Op(instr);
        }

        /// Add a floating point instruction
        /// \param bitWidth the bit width of the type
        /// \param value the constant value
        /// \return instruction reference
        InstructionRef <LiteralInstruction> FP(uint8_t bitWidth, double value) {
            return FP(map->AllocID(), bitWidth, value);
        }

        /// Emit an unexposed instruction
        /// \param result optional, the result id
        /// \param backendOpCode the unexposed opcode value
        /// \return instruction reference
        InstructionRef <UnexposedInstruction> Unexposed(ID result, uint32_t backendOpCode) {
            UnexposedInstruction instr{};
            instr.opCode = OpCode::Unexposed;
            instr.result = result;
            return Op(instr);
        }

        /// Load an address
        /// \param address the address to be loaded
        /// \return instruction reference
        InstructionRef <LoadInstruction> Load(ID address) {
            LoadInstruction instr{};
            instr.opCode = OpCode::Load;
            instr.address = address;
            instr.result = map->AllocID();
            return Op(instr);
        }

        /// Store to an address
        /// \param address the address to be stored to
        /// \param value the value to be stored
        /// \return instruction reference
        InstructionRef <StoreInstruction> Store(ID address, ID value) {
            StoreInstruction instr{};
            instr.result = InvalidID;
            instr.opCode = OpCode::Store;
            instr.address = address;
            instr.value = value;
            return Op(instr);
        }

        /// Store an element to a buffer
        /// \param buffer the destination buffer
        /// \param index the index of the element
        /// \param value the element data
        /// \return instruction reference
        InstructionRef <StoreBufferInstruction> StoreBuffer(ID buffer, ID index, ID value) {
            StoreBufferInstruction instr{};
            instr.opCode = OpCode::StoreBuffer;
            instr.buffer = buffer;
            instr.index = index;
            instr.value = value;
            instr.result = InvalidID;
            return Op(instr);
        }

        /// Binary add two values
        /// \param result the result id
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        InstructionRef <AddInstruction> Add(ID result, ID lhs, ID rhs) {
            AddInstruction instr{};
            instr.opCode = OpCode::Add;
            instr.result = result;
            instr.lhs = lhs;
            instr.rhs = rhs;
            instr.result = InvalidID;
            return Op(instr);
        }

        /// Binary add two values
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        InstructionRef <AddInstruction> Add(ID lhs, ID rhs) {
            return Add(map->AllocID(), lhs, rhs);
        }

        /// Is this emitter good?
        bool Good() const {
            return program && basicBlock;
        }

        /// Get the current program
        Program *GetProgram() const {
            return program;
        }

        /// Get the current basic block
        BasicBlock *GetBasicBlock() const {
            return basicBlock;
        }

    private:
        /// Perform the operation
        template<typename T>
        InstructionRef <T> Op(const T &instruction) {
            return OP::template Op<T>(basicBlock, insertionPoint, instruction);
        }

    private:
        /// Current insertion point
        Opaque insertionPoint{};

        /// Current program
        Program *program{nullptr};

        /// Current identifier map
        IdentifierMap *map{nullptr};

        /// Current basic block
        BasicBlock *basicBlock{nullptr};
    };
}
