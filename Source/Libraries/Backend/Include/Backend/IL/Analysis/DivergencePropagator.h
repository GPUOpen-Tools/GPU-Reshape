//
// The MIT License (MIT)
//
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
#include <Backend/IL/Analysis/ISimulationPropagator.h>
#include <Backend/IL/Analysis/WorkGroupDivergence.h>

namespace IL {
    class DivergencePropagator : public ISimulationPropagator {
    public:
        COMPONENT(DivergencePropagator);

        /// Constructor
        /// \param program program to propagate divergence for
        /// \param function function to propagate divergence for
        DivergencePropagator(Program& program, Function& function) : program(program), function(function) {

        }

        /// Get the divergence of a value
        WorkGroupDivergence GetDivergence(ID id) const {
            return divergenceValues.at(id);
        }

    public:
        /// Install this propagator
        bool Install() override {
            divergenceValues.resize(program.GetIdentifierMap().GetMaxID(), WorkGroupDivergence::Unknown);

            // Mark constants as uniform
            for (const Constant *constant: program.GetConstants()) {
                if (constant->IsSymbolic()) {
                    continue;
                }

                divergenceValues.at(constant->id) = WorkGroupDivergence::Uniform;
            }

            // Mark all constant initialized global variable as uniform
            for (const Backend::IL::Variable *variable: program.GetVariableList()) {
                if (!variable->initializer) {
                    continue;
                }

                divergenceValues.at(variable->id) = WorkGroupDivergence::Uniform;
            }

            // Mark all function inputs as divergent
            for (const Backend::IL::Variable *variable: function.GetParameters()) {
                divergenceValues.at(variable->id) = WorkGroupDivergence::Divergent;
            }

            // OK
            return true;
        }

        /// Propagate an instruction
        void PropagateInstruction(Backend::IL::PropagationResult, const BasicBlock*, const Instruction* instr, const BasicBlock*) override {
            switch (instr->opCode) {
                default: {
                    // Result-less instructions are ignored
                    if (instr->result == InvalidID) {
                        return;
                    }

                    return PropagateResultInstruction(instr);
                }
                case OpCode::AddressChain: {
                    // Chains themselves, i.e. the memory addresses, are not known
                    divergenceValues[instr->result] = WorkGroupDivergence::Unknown;
                    break;
                }
                case OpCode::Load: {
                    return PropagateLoadInstruction(instr->As<LoadInstruction>());
                }
                case OpCode::Store: {
                    return PropagateStoreInstruction(instr->As<StoreInstruction>());
                }
                case OpCode::Phi: {
                    return PropagatePhiInstruction(instr->As<PhiInstruction>());
                }
            }

        }

        /// Propagate all loop side-effects
        void PropagateLoopEffects(const Loop* loop) override {
            /** poof */
        }

    private:
        void PropagateResultInstruction(const Instruction *instr) {
            divergenceValues[instr->result] = GetInstructionDivergence(instr);
        }

        void PropagateLoadInstruction(const LoadInstruction *instr) {
            // TODO: Actual memory pattersn
            divergenceValues[instr->result] = divergenceValues[instr->address];
        }

        void PropagateStoreInstruction(const StoreInstruction *instr) {
            // TODO: Actual memory pattersn
            divergenceValues[instr->address] = divergenceValues[instr->value];
        }

        void PropagatePhiInstruction(const PhiInstruction *instr) {
            // TODO: Phi divergence analysis
        }

        /// Get the divergence of an instruction
        /// \param instr propagated instruction
        /// \return resulting divergence
        WorkGroupDivergence GetInstructionDivergence(const Instruction* instr) {
            // Instructions may converge divergent values
            if (IsConveringInstruction(instr)) {
                return WorkGroupDivergence::Uniform;
            }

            // Determine the base divergence, that is, the divergence of the instruction type itself
            WorkGroupDivergence baseDivergence = GetBaseDivergence(instr);

            // If divergent, don't check operands
            if (baseDivergence == WorkGroupDivergence::Divergent) {
                return baseDivergence;
            }

            // Attributes
            bool isAnyUnknown = false;
            bool isAnyDivergent = false;

            // Collect operand attributes
            Backend::IL::VisitOperands(instr, [&](IL::ID id) {
                isAnyUnknown |= divergenceValues[id] == WorkGroupDivergence::Unknown;
                isAnyDivergent |= divergenceValues[id] == WorkGroupDivergence::Divergent;
            });

            // If any are divergent, presume divergent
            if (isAnyDivergent) {
                return WorkGroupDivergence::Divergent;
            }

            // If any are unknown, we cannot make any presumption
            if (isAnyUnknown) {
                return WorkGroupDivergence::Unknown;
            }

            // Assume uniform
            return WorkGroupDivergence::Uniform;
        }

        /// Check if an instruction is converging
        /// \param instr instruction to query
        /// \return convergence
        bool IsConveringInstruction(const Instruction * instr) {
            switch (instr->opCode) {
                default: {
                    return false;
                }
                case OpCode::WaveReadFirst: {
                    return true;
                }
            }
        }

        /// Get the base divergence of an instruction
        /// This does not include operands or any carried / associated divergence
        /// \param instr instruction to query
        /// \return divergence
        WorkGroupDivergence GetBaseDivergence(const Instruction* instr) {
            switch (instr->opCode) {
                default: {
                    return WorkGroupDivergence::Uniform;
                }

                /** Load operations to external memory are always divergent */
                case OpCode::Load: {
                    auto* type = program.GetTypeMap().GetType(instr->As<LoadInstruction>()->address)->As<Backend::IL::PointerType>();
                    return AsDivergence(
                        type->addressSpace == Backend::IL::AddressSpace::Buffer ||
                        type->addressSpace == Backend::IL::AddressSpace::Texture ||
                        type->addressSpace == Backend::IL::AddressSpace::Resource
                    );
                }

                /** Atomic operations are always divergent */
                case OpCode::AtomicOr:
                case OpCode::AtomicXOr:
                case OpCode::AtomicAnd:
                case OpCode::AtomicAdd:
                case OpCode::AtomicMin:
                case OpCode::AtomicMax:
                case OpCode::AtomicExchange:
                case OpCode::AtomicCompareExchange: {
                    return WorkGroupDivergence::Divergent;
                }

                /** Resource operations are always divergent */
                case OpCode::SampleTexture:
                case OpCode::LoadTexture:
                case OpCode::LoadBuffer: {
                    return WorkGroupDivergence::Divergent;
                }
            }
        }

    private:
        /// Outer program
        Program& program;

        /// Source function
        Function& function;

        /// All propagated divergences (result wise lookup)
        std::vector<WorkGroupDivergence> divergenceValues;

        /// Constant analysis
        ComRef<ConstantPropagator> ConstantPropagator;
    };
}
