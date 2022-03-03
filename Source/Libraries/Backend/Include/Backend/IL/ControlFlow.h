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
