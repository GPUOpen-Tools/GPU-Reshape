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
#include "Instruction.h"
#include "BasicBlock.h"
#include "Program.h"
#include "TypeResult.h"
#include "ControlFlow.h"

// Common
#include <Common/Alloca.h>

namespace IL {
    namespace Op {
        /// Append operation
        struct Append {
            using Opaque = ConstOpaqueInstructionRef;

            /// Operation
            template<typename T>
            static BasicBlock::TypedIterator <T> Op(BasicBlock *basicBlock, Opaque &insertionPoint, const T &instruction) {
                if (insertionPoint.IsValid()) {
                    auto value = basicBlock->Insert(insertionPoint, instruction);
                    insertionPoint = std::next(value);
                    return value;
                } else {
                    auto value = basicBlock->Append(instruction);
                    return value;
                }
            }
        };

        /// Replacement operation
        struct Replace {
            using Opaque = OpaqueInstructionRef;

            /// Operation
            template<typename T>
            static BasicBlock::TypedIterator <T> Op(BasicBlock *basicBlock, Opaque &insertionPoint, const T &instruction) {
                ASSERT(insertionPoint.IsValid(), "Must have insertion point");

                auto value = basicBlock->Replace(insertionPoint, instruction);
                insertionPoint = value;
                return value;
            }
        };

        /// Instrumentation operation
        struct Instrument {
            using Opaque = OpaqueInstructionRef;

            /// Operation
            template<typename T>
            static BasicBlock::TypedIterator <T> Op(BasicBlock *basicBlock, Opaque &insertionPoint, T &instruction) {
                ASSERT(insertionPoint.IsValid(), "Must have insertion point");

                // Instrumentation must inherit the source instruction for specialized backend operands
                //  ? Not all instruction parameters are exposed, and altering these may change the intended behaviour
                instruction.source = InstructionRef<>(insertionPoint)->source.Modify();

                auto value = basicBlock->Replace(insertionPoint, instruction);
                insertionPoint = value;
                return value;
            }
        };
    }

    /// Emitter, easy instruction emitting
    template<typename OP = Op::Append>
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

        /// Constructor
        Emitter(Program &program, const Opaque &ref = {}) {
            SetProgram(&program);
            SetBasicBlock(ref.basicBlock);
            SetInsertionPoint(ref);
        }

        /// Set the current program
        void SetProgram(Program *value) {
            program = value;
            capabilityTable = &program->GetCapabilityTable();
            map = &value->GetIdentifierMap();
            typeMap = &value->GetTypeMap();
        }

        /// Set the current basic block
        void SetBasicBlock(BasicBlock *value) {
            basicBlock = value;
        }

        /// Set the insertion point
        void SetInsertionPoint(const Opaque &ref) {
            insertionPoint = ref;
        }

        /// Get the insertion point
        Opaque GetInsertionPoint() {
            return insertionPoint;
        }

        /// Add a new flag
        void AddBlockFlag(BasicBlockFlagSet flags) {
            basicBlock->AddFlag(flags);
        }

        /// Add an integral instruction
        /// \param bitWidth the bit width of the type
        /// \param value the constant value
        /// \return instruction reference
        BasicBlock::TypedIterator <LiteralInstruction> Integral(uint8_t bitWidth, int64_t value, bool signedness) {
            LiteralInstruction instr{};
            instr.opCode = OpCode::Literal;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.type = LiteralType::Int;
            instr.signedness = signedness;
            instr.bitWidth = bitWidth;
            instr.value.integral = value;
            return Op(instr);
        }

        /// Add an int constant
        /// \param bitWidth the bit width of the type
        /// \param value the constant value
        /// \return instruction reference
        BasicBlock::TypedIterator <LiteralInstruction> Int(uint8_t bitWidth, int64_t value) {
            return Integral(bitWidth, value, true);
        }

        /// Add a uint constant
        /// \param bitWidth the bit width of the type
        /// \param value the constant value
        /// \return instruction reference
        BasicBlock::TypedIterator <LiteralInstruction> UInt(uint8_t bitWidth, int64_t value) {
            return Integral(bitWidth, value, false);
        }

        /// Add a 32 bit integral instruction
        /// \param value the constant value
        /// \return instruction reference
        BasicBlock::TypedIterator <LiteralInstruction> Int32(int32_t value) {
            return Int(32, value);
        }

        /// Add a 16 bit integral instruction
        /// \param value the constant value
        /// \return instruction reference
        BasicBlock::TypedIterator <LiteralInstruction> Int16(int16_t value) {
            return Int(16, value);
        }

        /// Add a 8 bit integral instruction
        /// \param value the constant value
        /// \return instruction reference
        BasicBlock::TypedIterator <LiteralInstruction> Int8(int8_t value) {
            return Int(8, value);
        }

        /// Add a 32 bit integral instruction
        /// \param value the constant value
        /// \return instruction reference
        BasicBlock::TypedIterator <LiteralInstruction> UInt32(uint32_t value) {
            return UInt(32, value);
        }

        /// Add a 16 bit integral instruction
        /// \param value the constant value
        /// \return instruction reference
        BasicBlock::TypedIterator <LiteralInstruction> UInt16(uint16_t value) {
            return UInt(16, value);
        }

        /// Add a 8 bit integral instruction
        /// \param value the constant value
        /// \return instruction reference
        BasicBlock::TypedIterator <LiteralInstruction> UInt8(uint8_t value) {
            return UInt(8, value);
        }

        /// Add a floating point instruction
        /// \param result the result id
        /// \param bitWidth the bit width of the type
        /// \param value the constant value
        /// \return instruction reference
        BasicBlock::TypedIterator <LiteralInstruction> FP(uint8_t bitWidth, double value) {
            LiteralInstruction instr{};
            instr.opCode = OpCode::Literal;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.type = LiteralType::FP;
            instr.signedness = true;
            instr.bitWidth = bitWidth;
            instr.value.fp = value;
            return Op(instr);
        }

        /// Load an address
        /// \param address the address to be loaded
        /// \return instruction reference
        BasicBlock::TypedIterator <LoadInstruction> Load(ID address) {
            ASSERT(IsMapped(address), "Unmapped identifier");

            LoadInstruction instr{};
            instr.opCode = OpCode::Load;
            instr.source = Source::Invalid();
            instr.address = address;
            instr.result = map->AllocID();
            return Op(instr);
        }

        /// Store to an address
        /// \param address the address to be stored to
        /// \param value the value to be stored
        /// \return instruction reference
        BasicBlock::TypedIterator <StoreInstruction> Store(ID address, ID value) {
            ASSERT(IsMapped(address) && IsMapped(value), "Unmapped identifier");

            StoreInstruction instr{};
            instr.opCode = OpCode::Store;
            instr.source = Source::Invalid();
            instr.result = InvalidID;
            instr.address = address;
            instr.value = value;
            return Op(instr);
        }

        /// Store an element to a buffer
        /// \param buffer the destination buffer
        /// \param index the index of the element
        /// \param value the element data
        /// \return instruction reference
        BasicBlock::TypedIterator <StoreBufferInstruction> StoreBuffer(ID buffer, ID index, ID value) {
            ASSERT(IsMapped(buffer) && IsMapped(index) && IsMapped(value), "Unmapped identifier");

            StoreBufferInstruction instr{};
            instr.opCode = OpCode::StoreBuffer;
            instr.source = Source::Invalid();
            instr.buffer = buffer;
            instr.index = index;
            instr.value = value;
            instr.mask = ComponentMask::All;
            instr.result = InvalidID;
            return Op(instr);
        }

        /// Load an element from a buffer
        /// \param buffer the destination buffer
        /// \param index the index of the element
        /// \return instruction reference
        BasicBlock::TypedIterator <LoadBufferInstruction> LoadBuffer(ID buffer, ID index) {
            ASSERT(IsMapped(buffer) && IsMapped(index), "Unmapped identifier");

            LoadBufferInstruction instr{};
            instr.opCode = OpCode::LoadBuffer;
            instr.source = Source::Invalid();
            instr.buffer = buffer;
            instr.index = index;
            instr.offset = InvalidID;
            instr.result = map->AllocID();
            return Op(instr);
        }

        /// Get the size of a resource
        /// \param resource the resource to be queried
        /// \return instruction reference
        BasicBlock::TypedIterator <ResourceSizeInstruction> ResourceSize(ID resource) {
            ASSERT(IsMapped(resource), "Unmapped identifier");

            ResourceSizeInstruction instr{};
            instr.opCode = OpCode::ResourceSize;
            instr.source = Source::Invalid();
            instr.resource = resource;
            instr.result = map->AllocID();
            return Op(instr);
        }

        /// Get the identifier of a resource
        /// \param resource the resource to be queried
        /// \return instruction reference
        BasicBlock::TypedIterator <ResourceTokenInstruction> ResourceToken(ID resource) {
            ASSERT(IsMapped(resource), "Unmapped identifier");

            ResourceTokenInstruction instr{};
            instr.opCode = OpCode::ResourceToken;
            instr.source = Source::Invalid();
            instr.resource = resource;
            instr.result = map->AllocID();
            return Op(instr);
        }

        /// Binary add two values
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <BitCastInstruction> BitCast(ID value, const Backend::IL::Type* type) {
            ASSERT(IsMapped(value), "Unmapped identifier");

            BitCastInstruction instr{};
            instr.opCode = OpCode::BitCast;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.value = value;
            return Op(instr, type);
        }

        /// Get the address of a composite element
        /// \param composite the base composite address
        /// \param index the uniform index
        /// \return instruction reference
        template<typename... IX>
        BasicBlock::TypedIterator <AddressChainInstruction> AddressOf(ID composite, IX... ix) {
            ASSERT(IsMapped(composite), "Unmapped identifier");

            // Fold it down
            IL::ID chains[] = {ix...};

            auto instr = ALLOCA_SIZE(IL::AddressChainInstruction, IL::AddressChainInstruction::GetSize(1));
            instr->opCode = OpCode::AddressChain;
            instr->source = Source::Invalid();
            instr->result = map->AllocID();
            instr->composite = composite;
            instr->chains.count = sizeof...(IX);

            // Write all chains
            for (uint32_t i = 0; i < static_cast<uint32_t>(instr->chains.count); i++) {
                instr->chains[i].index = chains[i];
            }

            return Op(*instr);
        }

        /// Extract a value from a composite
        /// \param composite the base composite
        /// \param index the index
        /// \return instruction reference
        BasicBlock::TypedIterator <ExtractInstruction> Extract(ID composite, uint32_t index) {
            ASSERT(IsMapped(composite) && IsMapped(index), "Unmapped identifier");

            ExtractInstruction instr{};
            instr.opCode = OpCode::Extract;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.composite = composite;
            instr.index = index;
            return Op(instr);
        }

        /// Insert a value to a composite
        /// \param composite the base composite
        /// \param index the value to be inserted
        /// \return instruction reference
        BasicBlock::TypedIterator <InsertInstruction> Insert(ID composite, ID value) {
            ASSERT(IsMapped(composite) && IsMapped(value), "Unmapped identifier");

            InsertInstruction instr{};
            instr.opCode = OpCode::Insert;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.composite = composite;
            instr.value = value;
            return Op(instr);
        }

        /// Select a value
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <SelectInstruction> Select(ID condition, ID pass, ID fail) {
            ASSERT(IsMapped(condition) && IsMapped(pass) && IsMapped(fail), "Unmapped identifier");

            SelectInstruction instr{};
            instr.opCode = OpCode::Select;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.condition = condition;
            instr.pass = pass;
            instr.fail = fail;
            return Op(instr);
        }

        /// Binary add two values
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <RemInstruction> Rem(ID lhs, ID rhs) {
            ASSERT(IsMapped(lhs) && IsMapped(rhs), "Unmapped identifier");

            RemInstruction instr{};
            instr.opCode = OpCode::Rem;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.lhs = lhs;
            instr.rhs = rhs;
            return Op(instr);
        }

        /// Binary add two values
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <AddInstruction> Add(ID lhs, ID rhs) {
            ASSERT(IsMapped(lhs) && IsMapped(rhs), "Unmapped identifier");

            AddInstruction instr{};
            instr.opCode = OpCode::Add;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.lhs = lhs;
            instr.rhs = rhs;
            return Op(instr);
        }

        /// Binary sub two values
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <SubInstruction> Sub(ID lhs, ID rhs) {
            ASSERT(IsMapped(lhs) && IsMapped(rhs), "Unmapped identifier");

            SubInstruction instr{};
            instr.opCode = OpCode::Sub;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.lhs = lhs;
            instr.rhs = rhs;
            return Op(instr);
        }

        /// Binary div two values
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <DivInstruction> Div(ID lhs, ID rhs) {
            ASSERT(IsMapped(lhs) && IsMapped(rhs), "Unmapped identifier");

            DivInstruction instr{};
            instr.opCode = OpCode::Div;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.lhs = lhs;
            instr.rhs = rhs;
            return Op(instr);
        }

        /// Binary mul two values
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <MulInstruction> Mul(ID lhs, ID rhs) {
            ASSERT(IsMapped(lhs) && IsMapped(rhs), "Unmapped identifier");

            MulInstruction instr{};
            instr.opCode = OpCode::Mul;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.lhs = lhs;
            instr.rhs = rhs;
            return Op(instr);
        }

        /// Check for equality
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <EqualInstruction> Equal(ID lhs, ID rhs) {
            ASSERT(IsMapped(lhs) && IsMapped(rhs), "Unmapped identifier");

            EqualInstruction instr{};
            instr.opCode = OpCode::Equal;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.lhs = lhs;
            instr.rhs = rhs;
            return Op(instr);
        }

        /// Check if two values are not equal
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <NotEqualInstruction> NotEqual(ID lhs, ID rhs) {
            ASSERT(IsMapped(lhs) && IsMapped(rhs), "Unmapped identifier");

            NotEqualInstruction instr{};
            instr.opCode = OpCode::NotEqual;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.lhs = lhs;
            instr.rhs = rhs;
            return Op(instr);
        }

        /// Check if one value is greater than another
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <GreaterThanInstruction> GreaterThan(ID lhs, ID rhs) {
            ASSERT(IsMapped(lhs) && IsMapped(rhs), "Unmapped identifier");

            GreaterThanInstruction instr{};
            instr.opCode = OpCode::GreaterThan;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.lhs = lhs;
            instr.rhs = rhs;
            return Op(instr);
        }

        /// Check if one value is greater than or equal to another
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <GreaterThanEqualInstruction> GreaterThanEqual(ID lhs, ID rhs) {
            ASSERT(IsMapped(lhs) && IsMapped(rhs), "Unmapped identifier");

            GreaterThanEqualInstruction instr{};
            instr.opCode = OpCode::GreaterThanEqual;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.lhs = lhs;
            instr.rhs = rhs;
            return Op(instr);
        }

        /// Check if one value is less than another
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <LessThanInstruction> LessThan(ID lhs, ID rhs) {
            ASSERT(IsMapped(lhs) && IsMapped(rhs), "Unmapped identifier");

            LessThanInstruction instr{};
            instr.opCode = OpCode::LessThan;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.lhs = lhs;
            instr.rhs = rhs;
            return Op(instr);
        }

        /// Check if one value is less than or equal to another
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <LessThanEqualInstruction> LessThanEqual(ID lhs, ID rhs) {
            ASSERT(IsMapped(lhs) && IsMapped(rhs), "Unmapped identifier");

            LessThanEqualInstruction instr{};
            instr.opCode = OpCode::LessThanEqual;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.lhs = lhs;
            instr.rhs = rhs;
            return Op(instr);
        }

        /// Check if a value is infinite
        /// \param value value operand
        /// \return instruction reference
        BasicBlock::TypedIterator <IsInfInstruction> IsInf(ID value) {
            ASSERT(IsMapped(value), "Unmapped identifier");

            IsInfInstruction instr{};
            instr.opCode = OpCode::IsInf;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.value = value;
            return Op(instr);
        }

        /// Check if a value is NaN
        /// \param value value operand
        /// \return instruction reference
        BasicBlock::TypedIterator <IsNaNInstruction> IsNaN(ID value) {
            ASSERT(IsMapped(value), "Unmapped identifier");

            IsNaNInstruction instr{};
            instr.opCode = OpCode::IsNaN;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.value = value;
            return Op(instr);
        }

        /// Perform a bitwise or
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <BitOrInstruction> BitOr(ID lhs, ID rhs) {
            ASSERT(IsMapped(lhs) && IsMapped(rhs), "Unmapped identifier");

            BitOrInstruction instr{};
            instr.opCode = OpCode::BitOr;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.lhs = lhs;
            instr.rhs = rhs;
            return Op(instr);
        }

        /// Perform a bitwise and
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <BitAndInstruction> BitAnd(ID lhs, ID rhs) {
            ASSERT(IsMapped(lhs) && IsMapped(rhs), "Unmapped identifier");

            BitAndInstruction instr{};
            instr.opCode = OpCode::BitAnd;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.lhs = lhs;
            instr.rhs = rhs;
            return Op(instr);
        }

        /// Perform a logical or
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <OrInstruction> Or(ID lhs, ID rhs) {
            ASSERT(IsMapped(lhs) && IsMapped(rhs), "Unmapped identifier");

            OrInstruction instr{};
            instr.opCode = OpCode::Or;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.lhs = lhs;
            instr.rhs = rhs;
            return Op(instr);
        }

        /// Perform a logical and
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <AndInstruction> And(ID lhs, ID rhs) {
            ASSERT(IsMapped(lhs) && IsMapped(rhs), "Unmapped identifier");

            AndInstruction instr{};
            instr.opCode = OpCode::And;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.lhs = lhs;
            instr.rhs = rhs;
            return Op(instr);
        }

        /// Check if all components may be evaluated to true
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <AllInstruction> All(ID value) {
            ASSERT(IsMapped(value), "Unmapped identifier");

            AllInstruction instr{};
            instr.opCode = OpCode::All;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.value = value;
            return Op(instr);
        }

        /// Check if any components may be evaluated to true
        /// \param lhs lhs operand
        /// \param rhs rhs operand
        /// \return instruction reference
        BasicBlock::TypedIterator <AnyInstruction> Any(ID value) {
            ASSERT(IsMapped(value), "Unmapped identifier");

            AnyInstruction instr{};
            instr.opCode = OpCode::Any;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.value = value;
            return Op(instr);
        }

        /// Perform a bitwise left shift
        /// \param lhs value the value to be shifted
        /// \param rhs shift the bit shift count
        /// \return instruction reference
        BasicBlock::TypedIterator <BitShiftLeftInstruction> BitShiftLeft(ID value, ID shift) {
            ASSERT(IsMapped(value) && IsMapped(shift), "Unmapped identifier");

            BitShiftLeftInstruction instr{};
            instr.opCode = OpCode::BitShiftLeft;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.value = value;
            instr.shift = shift;
            return Op(instr);
        }

        /// Perform a bitwise right shift
        /// \param lhs value the value to be shifted
        /// \param rhs shift the bit shift count
        /// \return instruction reference
        BasicBlock::TypedIterator <BitShiftRightInstruction> BitShiftRight(ID value, ID shift) {
            ASSERT(IsMapped(value) && IsMapped(shift), "Unmapped identifier");

            BitShiftRightInstruction instr{};
            instr.opCode = OpCode::BitShiftRight;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.value = value;
            instr.shift = shift;
            return Op(instr);
        }

        /// Branch to a block
        /// \param branch the destination block
        /// \return instruction reference
        BasicBlock::TypedIterator <BranchInstruction> Branch(BasicBlock* branch, ControlFlow controlFlow = ControlFlow::None()) {
            ASSERT(branch, "Invalid branch");

            BranchInstruction instr{};
            instr.opCode = OpCode::Branch;
            instr.source = Source::Invalid();
            instr.result = IL::InvalidID;
            instr.branch = branch->GetID();
            instr.controlFlow = controlFlow;
            return Op(instr);
        }

        /// Conditionally branch to a block
        /// \param cond the condition
        /// \param pass the block branched to if cond is true
        /// \param fail the block branched ot if cond is false
        /// \param merge the structured control flow
        /// \return instruction reference
        BasicBlock::TypedIterator <BranchConditionalInstruction> BranchConditional(ID cond, BasicBlock* pass, BasicBlock* fail, ControlFlow controlFlow) {
            ASSERT(IsMapped(cond), "Unmapped identifier");
            ASSERT(pass && fail, "Invalid branch");

            BranchConditionalInstruction instr{};
            instr.opCode = OpCode::BranchConditional;
            instr.source = Source::Invalid();
            instr.result = IL::InvalidID;
            instr.cond = cond;
            instr.pass = pass->GetID();
            instr.fail = fail->GetID();
            instr.controlFlow = controlFlow;
            return Op(instr);
        }

        /// Add return instruction
        /// \param value optional value to return
        /// \return instruction reference
        BasicBlock::TypedIterator <ReturnInstruction> Return(ID value = IL::InvalidID) {
            ReturnInstruction instr{};
            instr.opCode = OpCode::Return;
            instr.source = Source::Invalid();
            instr.result = IL::InvalidID;
            instr.value = value;
            return Op(instr);
        }

        /// Add phi instruction
        /// \param first first case basic block
        /// \param firstValue first case value produced by first basic block
        /// \param second second case basic block
        /// \param secondValue second case value produced by first basic block
        /// \return instruction reference
        BasicBlock::TypedIterator <PhiInstruction> Phi(BasicBlock* first, ID firstValue, BasicBlock* second, ID secondValue) {
            return Phi(map->AllocID(), first, firstValue, second, secondValue);
        }

        /// Add phi instruction
        /// \param result result id
        /// \param first first case basic block
        /// \param firstValue first case value produced by first basic block
        /// \param second second case basic block
        /// \param secondValue second case value produced by first basic block
        /// \return instruction reference
        BasicBlock::TypedIterator <PhiInstruction> Phi(IL::ID result, BasicBlock* first, ID firstValue, BasicBlock* second, ID secondValue) {
            ASSERT(IsMapped(firstValue) && IsMapped(secondValue), "Unmapped identifier");
            ASSERT(first && second, "Invalid branch");

            auto instr = ALLOCA_SIZE(IL::PhiInstruction, IL::PhiInstruction::GetSize(2u));
            instr->opCode = OpCode::Phi;
            instr->source = Source::Invalid();
            instr->result = result;
            instr->values.count = 2u;
            instr->values[0].branch = first->GetID();
            instr->values[0].value = firstValue;
            instr->values[1].branch = second->GetID();
            instr->values[1].value = secondValue;
            return Op(*instr);
        }

        /// Add phi instruction
        /// \param result result id
        /// \param first first case basic block
        /// \param firstValue first case value produced by first basic block
        /// \param second second case basic block
        /// \param secondValue second case value produced by first basic block
        /// \return instruction reference
        BasicBlock::TypedIterator <PhiInstruction> Phi(IL::ID result, uint32_t count, PhiValue* values) {
            auto instr = ALLOCA_SIZE(IL::PhiInstruction, IL::PhiInstruction::GetSize(count));
            instr->opCode = OpCode::Phi;
            instr->source = Source::Invalid();
            instr->result = result;
            instr->values.count = count;

            // Assign all values
            for (uint32_t i = 0; i < count; i++) {
                ASSERT(IsMapped(values[i].branch) && IsMapped(values[i].value), "Unmapped identifier");
                instr->values[i].branch = values[i].branch;
                instr->values[i].value = values[i].value;
            }

            return Op(*instr);
        }

        /// Perform an atomic / interlocked or
        /// \param address base address
        /// \param value value
        /// \return original value
        BasicBlock::TypedIterator<AtomicOrInstruction> AtomicOr(ID address, ID value) {
            ASSERT(IsMapped(address) && IsMapped(value), "Unmapped identifier");

            AtomicOrInstruction instr{};
            instr.opCode = OpCode::AtomicOr;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.address = address;
            instr.value = value;
            return Op(instr);
        }

        /// Perform an atomic / interlocked exclusive or
        /// \param address base address
        /// \param value value
        /// \return original value
        BasicBlock::TypedIterator<AtomicXOrInstruction> AtomicXOr(ID address, ID value) {
            ASSERT(IsMapped(address) && IsMapped(value), "Unmapped identifier");

            AtomicXOrInstruction instr{};
            instr.opCode = OpCode::AtomicXOr;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.address = address;
            instr.value = value;
            return Op(instr);
        }

        /// Perform an atomic / interlocked and
        /// \param address base address
        /// \param value value
        /// \return original value
        BasicBlock::TypedIterator<AtomicAndInstruction> AtomicAnd(ID address, ID value) {
            ASSERT(IsMapped(address) && IsMapped(value), "Unmapped identifier");

            AtomicAndInstruction instr{};
            instr.opCode = OpCode::AtomicAnd;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.address = address;
            instr.value = value;
            return Op(instr);
        }

        /// Perform an atomic / interlocked add
        /// \param address base address
        /// \param value value
        /// \return original value
        BasicBlock::TypedIterator<AtomicAddInstruction> AtomicAdd(ID address, ID value) {
            ASSERT(IsMapped(address) && IsMapped(value), "Unmapped identifier");

            AtomicAddInstruction instr{};
            instr.opCode = OpCode::AtomicAdd;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.address = address;
            instr.value = value;
            return Op(instr);
        }

        /// Perform an atomic / interlocked min
        /// \param address base address
        /// \param value value
        /// \return original value
        BasicBlock::TypedIterator<AtomicMinInstruction> AtomicMin(ID address, ID value) {
            ASSERT(IsMapped(address) && IsMapped(value), "Unmapped identifier");

            AtomicMinInstruction instr{};
            instr.opCode = OpCode::AtomicMin;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.address = address;
            instr.value = value;
            return Op(instr);
        }

        /// Perform an atomic / interlocked max
        /// \param address base address
        /// \param value value
        /// \return original value
        BasicBlock::TypedIterator<AtomicMaxInstruction> AtomicMax(ID address, ID value) {
            ASSERT(IsMapped(address) && IsMapped(value), "Unmapped identifier");

            AtomicMaxInstruction instr{};
            instr.opCode = OpCode::AtomicMax;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.address = address;
            instr.value = value;
            return Op(instr);
        }

        /// Perform an atomic / interlocked exchange
        /// \param address base address
        /// \param value value
        /// \return original value
        BasicBlock::TypedIterator<AtomicExchangeInstruction> AtomicExchange(ID address, ID value) {
            ASSERT(IsMapped(address) && IsMapped(value), "Unmapped identifier");

            AtomicExchangeInstruction instr{};
            instr.opCode = OpCode::AtomicExchange;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.address = address;
            instr.value = value;
            return Op(instr);
        }

        /// Perform an atomic / interlocked compare exchange
        /// \param address base address
        /// \param comparator base value to be compared against for successful exchange
        /// \param value value
        /// \return original value
        BasicBlock::TypedIterator<AtomicCompareExchangeInstruction> AtomicCompareExchange(ID address, ID comparator, ID value) {
            ASSERT(IsMapped(address) && IsMapped(comparator) && IsMapped(value), "Unmapped identifier");

            AtomicCompareExchangeInstruction instr{};
            instr.opCode = OpCode::AtomicCompareExchange;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();
            instr.address = address;
            instr.comparator = comparator;
            instr.value = value;
            return Op(instr);
        }

        /// Export a shader export
        /// \param exportID the allocation id for the export
        /// \param value the value to be exported
        /// \return instruction reference
        BasicBlock::TypedIterator <ExportInstruction> Export(ShaderExportID exportID, ID value) {
            ASSERT(IsMapped(value), "Unmapped identifier");

            auto instr = ALLOCA_SIZE(IL::ExportInstruction, IL::ExportInstruction::GetSize(1u));
            instr->opCode = OpCode::Export;
            instr->source = Source::Invalid();
            instr->result = map->AllocID();
            instr->exportID = exportID;
            instr->values.count = 1u;
            instr->values[0] = value;
            return Op(*instr);
        }

        /// Construct and export a shader export
        /// \param exportID the allocation id for the export
        /// \param value the value to be exported, constructed internally
        /// \return instruction reference
        template<typename T>
        BasicBlock::TypedIterator <ExportInstruction> Export(ShaderExportID exportID, const T& value) {
            // Query number of dwords requested
            uint32_t dwordCount{};
            value.Construct(*this, &dwordCount, nullptr);

            // Alloca on count
            auto instr = ALLOCA_SIZE(IL::ExportInstruction, IL::ExportInstruction::GetSize(dwordCount));
            instr->opCode = OpCode::Export;
            instr->source = Source::Invalid();
            instr->result = map->AllocID();
            instr->exportID = exportID;
            instr->values.count = dwordCount;

            // Fill dwords
            value.Construct(*this, &dwordCount, &instr->values[0]);
            return Op(*instr);
        }

        /// Alloca a varaible
        /// \param type the varaible type
        /// \return instruction reference
        BasicBlock::TypedIterator <AllocaInstruction> Alloca(const Backend::IL::Type* type) {
            ASSERT(IsMapped(type), "Unmapped identifier");

            AllocaInstruction instr{};
            instr.opCode = OpCode::Alloca;
            instr.source = Source::Invalid();
            instr.result = map->AllocID();

            return Op(instr, program->GetTypeMap().FindTypeOrAdd(Backend::IL::PointerType {
                .pointee = type,
                .addressSpace = Backend::IL::AddressSpace::Function
            }));
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

        /// Get the current iterator
        BasicBlock::Iterator GetIterator() const {
            return basicBlock->GetIterator(insertionPoint);
        }

    private:
        /// Check if an id is mapped
        bool IsMapped(ID id) const {
            return id != InvalidID;
        }

        /// Perform the operation
        template<typename T>
        BasicBlock::TypedIterator <T> Op(T &instruction) {
            // Set type of the instruction if relevant
            if (const Backend::IL::Type* type = Backend::IL::ResultOf(*program, &instruction)) {
                program->GetTypeMap().SetType(instruction.result, type);
            }

            // Perform the insertion
            return OP::template Op<T>(basicBlock, insertionPoint, instruction);
        }

        /// Perform the operation
        template<typename T>
        BasicBlock::TypedIterator <T> Op(T &instruction, const Backend::IL::Type* type) {
            // Set type of the instruction
            program->GetTypeMap().SetType(instruction.result, type);

            // Perform the insertion
            return OP::template Op<T>(basicBlock, insertionPoint, instruction);
        }

    private:
        /// Current insertion point
        Opaque insertionPoint{};

        /// Current program
        Program *program{nullptr};

        /// Current capability table
        CapabilityTable* capabilityTable{nullptr};

        /// Current identifier map
        IdentifierMap *map{nullptr};

        /// Current type map
        Backend::IL::TypeMap* typeMap{nullptr};

        /// Current basic block
        BasicBlock *basicBlock{nullptr};
    };
}
