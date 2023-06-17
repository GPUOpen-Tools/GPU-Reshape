// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
#include "BasicBlock.h"

namespace IL {
    struct ControlFlow {
        /// No structured control flow
        /// \return control flow
        static ControlFlow None() {
            return ControlFlow();
        }

        /// Create a structured selection control flow
        /// \param merge post merge block
        /// \return control flow
        static ControlFlow Selection(const BasicBlock* merge) {
            ControlFlow flow;
            flow.merge = merge;
            return flow;
        }

        /// Create a structured loop control flow
        /// \param merge post merge block
        /// \param _continue inner loop continuation block
        /// \return control flow
        static ControlFlow Loop(const BasicBlock* merge, const BasicBlock* _continue) {
            ControlFlow flow;
            flow.merge = merge;
            flow._continue = _continue;
            return flow;
        }

        /// Is this control flow structured?
        bool IsStructured() const {
            return merge != nullptr;
        }

        /// Implicit to branch
        operator BranchControlFlow() const {
            BranchControlFlow flow;
            flow.merge = merge ? merge->GetID() : InvalidID;
            flow._continue = _continue ? _continue->GetID() : InvalidID;
            return flow;
        }

        /// Selection and loop merge block
        ///  ? Essentially what is branched to after the if / switch / loop
        const BasicBlock* merge{nullptr};

        /// Loop continue block
        const BasicBlock* _continue{nullptr};
    };
}
