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
#include <Backend/IL/Utils/PropagationResult.h>

// Common
#include <Common/IComponent.h>

namespace Backend::IL {
    class PropagationEngine;
}

namespace IL {
    // Forward declarations
    struct BasicBlock;
    struct Instruction;
    struct Loop;

    class ISimulationPropagator : public IComponent {
    public:
        /// Install this propagator
        /// \param engine the propagation engine
        /// \return success state
        virtual bool Install(Backend::IL::PropagationEngine* engine) = 0;

        /// Propgate an instruction
        /// \param result propagator engine result
        /// \param block source block
        /// \param instr source instruction
        /// \param branchBlock branch chosen, if applicable
        virtual void PropagateInstruction(Backend::IL::PropagationResult result, const BasicBlock* block, const Instruction* instr, const BasicBlock* branchBlock) = 0;

        /// Propagate all side effects of a loop
        /// \param loop given loop
        virtual void PropagateLoopEffects(const Loop* loop) = 0;
    };
}
