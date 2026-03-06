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
#include <Backend/IL/Debug/DebugStack.h>
#include <Backend/IL/Emitters/Emitter.h>

// Common
#include <Common/Containers/LinearBlockAllocator.h>

namespace IL {
    class IDebugEmitter : public TComponent<IDebugEmitter> {
    public:
        COMPONENT(IDebugEmitter);

        /// Reconstruct the debugging value type
        /// @param program owning program
        /// @param instr instruction to reconstruct for
        /// @param arena shared arena allocator
        /// @param stack reconstructed stack
        virtual void GetStack(Program& program, const Instruction* instr, SmallArena& arena, DebugStack& stack) = 0;

        /// Reconstruct the debugging value
        /// @param emitter the emitter used for reconstruction
        /// @param value variable chosen to reconstruct
        /// @param instr instruction to reconstruct for
        /// @return invalid if failed
        virtual ID ReconstructValue(Emitter<>& emitter, const IL::DebugSingleValue& value, const Instruction* instr) = 0;
    };
}
