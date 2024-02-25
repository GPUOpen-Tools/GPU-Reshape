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
#include <Backend/IL/Analysis/SimulationAnalysis.h>

namespace IL {
    class InterproceduralSimulationAnalysis : public IProgramAnalysis {
    public:
        COMPONENT(InterproceduralSimulationAnalysis);

        /// Constructor
        /// \param program program to propagate divergence for
        InterproceduralSimulationAnalysis(Program& program) :
            program(program),
            constantMemory(program) {
        }

        /// Compute propagation of entire program
        bool Compute() override {
            // Install program wide memory
            if (!constantMemory.Install()) {
                return false;
            }

            // Setup all function simulators
            for (Function* function : program.GetFunctionList()) {
                ComRef analysis = function->GetAnalysisMap().FindPassOrAdd<SimulationAnalysis>(program, *function);
                if (!analysis) {
                    ASSERT(false, "Failed to set up analysis for function");
                    return false;
                }

                // Set the shared memory
                analysis->GetConstantPropagator().SetMemory(&constantMemory);
            }

            // Must have a single entry point
            Function* entryPoint = program.GetEntryPoint();
            if (!entryPoint) {
                return false;
            }

            // Must have analysis by now
            ComRef analysis = entryPoint->GetAnalysisMap().FindPass<SimulationAnalysis>();
            if (!analysis) {
                return false;
            }

            // Compute it
            if (!analysis->Compute()) {
                return false;
            }

            // Finally, composite all compile time ranges
            constantMemory.CompositeRanges();

            // OK
            return true;
        }

    private:
        /// Outer program
        Program& program;

        /// Shared constant memory
        ConstantPropagatorMemory constantMemory;
    };
}
