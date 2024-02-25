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
#include <Backend/IL/Analysis/IAnalysis.h>
#include <Backend/IL/Analysis/ISimulationPropagator.h>
#include <Backend/IL/Analysis/ConstantPropagator.h>

namespace IL {
    class SimulationAnalysis : public IFunctionAnalysis {
    public:
        COMPONENT(SimulationAnalysis);

        /// Constructor
        /// \param program program to propagate divergence for
        /// \param function function to propagate divergence for
        SimulationAnalysis(Program& program, Function& function) :
            program(program), function(function),
            propagationEngine(program, function),
            constantPropagator(program, function, propagationEngine) {
        }

        /// Compute constant propagation of a function
        bool Compute() override {
            // Setup constant analysis
            if (!constantPropagator.Install()) {
                return false;
            }

            // Notify propagators
            for (const ComRef<ISimulationPropagator>& propagator : propagators) {
                if (!propagator->Install(&propagationEngine)) {
                    return false;
                }
            }

            // Compute propagation
            propagationEngine.Compute(*this);

            // OK
            return true;
        }

        /// Get the underlying constant propagator
        /// This is guaranteed to exist for any simulator
        ConstantPropagator& GetConstantPropagator() {
            return constantPropagator;
        }

        /// Get the underlying constant propagator
        /// This is guaranteed to exist for any simulator
        const ConstantPropagator& GetConstantPropagator() const {
            return constantPropagator;
        }

        /// Find a propagator or construct it if it doesn't exist
        /// \param args all construction arguments
        /// \return nullptr if failed to create
        template<typename U, typename... AX>
        ComRef<U> FindPropagatorOrAdd(AX&&... args) {
            static_assert(std::is_base_of_v<ISimulationPropagator, U>, "Invalid propagator target");

            // Already exists?
            if (ComRef<U> propagator = FindPropagator<U>()) {
                return std::move(propagator);
            }

            // Create a new one
            ComRef<U> propagator = registry.New<U>(std::forward<AX>(args)...);
            propagators.push_back(propagator);
            return propagator;
        }

        /// Find a propagator or construct it if it doesn't exist
        /// \param args all construction arguments
        /// \return nullptr if failed to create
        template<typename U, typename... AX>
        ComRef<U> AddPropagator(AX&&... args) {
            static_assert(std::is_base_of_v<ISimulationPropagator, U>, "Invalid propagator target");
            ComRef<U> propagator = registry->New<U>(std::forward<AX>(args)...);
            propagators.push_back(propagator);
            return propagator;
        }

        /// Find an existing propagator
        /// \return nullptr if not found
        template<typename U>
        ComRef<U> FindPropagator() {
            static_assert(std::is_base_of_v<ISimulationPropagator, U>, "Invalid propagator target");
            for (const ComRef<ISimulationPropagator>& propagator : propagators) {
                if (propagator->componentId == U::kID) {
                    return propagator;
                }
            }

            // Not found
            return nullptr;
        }

    public:
        /// Propagate an instruction
        Backend::IL::PropagationResult PropagateInstruction(const BasicBlock* block, const Instruction* instr, const BasicBlock** branchBlock) {
            // Interproceduralism is handled by the simulator, not constant propagator
            if (auto call = instr->Cast<CallInstruction>()) {
                SimulateProcedure(block, call);
            }
            
            Backend::IL::PropagationResult result = constantPropagator.PropagateInstruction(block, instr, branchBlock);

            // Notify propagators
            for (const ComRef<ISimulationPropagator>& propagator : propagators) {
                propagator->PropagateInstruction(result, block, instr, *branchBlock);
            }

            // OK
            return result;
        }

        /// Propagate all loop side effects
        void PropagateLoopEffects(const Loop* loop) {
            constantPropagator.PropagateLoopEffects(loop);

            // Notify propagators
            for (const ComRef<ISimulationPropagator>& propagator : propagators) {
                propagator->PropagateLoopEffects(loop);
            }
        }

        /// Clear an instruction
        void ClearInstruction(const Instruction* instr) {
            constantPropagator.ClearInstruction(instr);
        }
        
    private:
        /// Simulate a procedure call
        /// \param block source block
        /// \param call source instruction
        void SimulateProcedure(const BasicBlock* block, const CallInstruction* call) {
            Function* target = program.GetFunctionList().GetFunction(call->target);
            if (!target) {
                ASSERT(false, "Invalid target");
                return;
            }

            // Use nested simulation passes
            ComRef analysis = target->GetAnalysisMap().FindPass<SimulationAnalysis>();
            if (!analysis) {
                ASSERT(false, "Failed to set up simulation for procedure");
                return;
            }

            // Propagate all state from the block to this propagator
            analysis->GetConstantPropagator().PropagateGlobalState(constantPropagator, block);

            // TODO: Erase previous state
            auto parameterIt = target->GetParameters().begin();

            // Assign all parameters
            for (uint32_t i = 0; i < call->arguments.count; i++) {
                const Backend::IL::Variable *parameter = *parameterIt;

                // Inform constant propagator of static store
                constantPropagator.StoreStatic(parameter->id, call->arguments[i]);

                // Notify propagators of static store
                for (const ComRef<ISimulationPropagator>& propagator : propagators) {
                    propagator->StoreStatic(parameter->id, call->arguments[i]);
                }

                // Advance parameter
                ASSERT(parameterIt < target->GetParameters().end(), "Out of bounds iterator");
                ++parameterIt;
            }

            // Try to simulate it
            if (!analysis->Compute()) {
                return;
            }
        }

    private:
        /// Outer program
        Program& program;

        /// Source function
        Function& function;

        /// Underlying propagation engine
        Backend::IL::PropagationEngine propagationEngine;

        /// Constant analysis
        ConstantPropagator constantPropagator;

        /// All user added propagators
        std::vector<ComRef<ISimulationPropagator>> propagators;
    };
}
