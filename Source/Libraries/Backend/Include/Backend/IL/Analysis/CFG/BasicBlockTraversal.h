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
#include <Backend/IL/BasicBlockList.h>

// Std
#include <stack>
#include <variant>
#include <vector>

/// Use stack based traversal
#define BASIC_BLOCK_TRAVERSAL_USE_STACK 1

namespace IL {
    class BasicBlockTraversal {
    public:
        using BlockView = std::vector<BasicBlock *>;

        /// Perform post-order traversal
        /// \param basicBlocks all basic blocks
        void PostOrder(BasicBlockList& basicBlocks) {
            Clear(basicBlocks);

            // Invoke traversal
#if BASIC_BLOCK_TRAVERSAL_USE_STACK
            TraversePostOrderStack(basicBlocks, basicBlocks.GetEntryPoint());
#else // BASIC_BLOCK_TRAVERSAL_USE_STACK
            TraversePostOrderRecursion(basicBlocks, basicBlocks.GetEntryPoint());
#endif // BASIC_BLOCK_TRAVERSAL_USE_STACK
        }

        /// Get the current traversal view
        const BlockView &GetView() const {
            return blocks;
        }

    private:
        /// Perform post-order traversal using recursion
        /// \param basicBlocks all basic blocks
        /// \param bb basic block to traverse
        void TraversePostOrderRecursion(BasicBlockList& basicBlocks, BasicBlock *bb) {
            // Attempt to acquire the basic block
            if (!Acquire(bb)) {
                return;
            }

            // Get terminator
            const Instruction *terminator = bb->GetTerminator();

            // Handle terminator
            switch (terminator->opCode) {
                default:
                    // ASSERT(false, "Unknown terminator");
                    break;
                case OpCode::Branch: {
                    auto *instr = terminator->As<BranchInstruction>();
                    TraversePostOrderRecursion(basicBlocks, basicBlocks.GetBlock(instr->branch));
                    break;
                }
                case OpCode::BranchConditional: {
                    auto *instr = terminator->As<BranchConditionalInstruction>();
                    TraversePostOrderRecursion(basicBlocks, basicBlocks.GetBlock(instr->pass));
                    TraversePostOrderRecursion(basicBlocks, basicBlocks.GetBlock(instr->fail));
                    break;
                }
                case OpCode::Switch: {
                    auto *instr = terminator->As<SwitchInstruction>();
                    TraversePostOrderRecursion(basicBlocks, basicBlocks.GetBlock(instr->_default));
                    for (uint32_t caseIndex = 0; caseIndex < instr->cases.count; caseIndex++) {
                        TraversePostOrderRecursion(basicBlocks, basicBlocks.GetBlock(instr->cases[caseIndex].branch));
                    }
                    break;
                }
                case OpCode::Return: {
                    break;
                }
            }

            // Mark as candidate
            blocks.push_back(bb);
        }
        
        /// Perform post-order traversal on the stack
        /// \param basicBlocks all basic blocks
        /// \param root basic block to traverse
        void TraversePostOrderStack(BasicBlockList& basicBlocks, BasicBlock *root) {
            std::stack<BasicBlock*> frontTraversal;

            // Acquire root
            frontTraversal.push(root);
            Acquire(root);

            // Process all front blocks
            while (!frontTraversal.empty()) {
                BasicBlock* bb = frontTraversal.top();
                frontTraversal.pop();

                // Order set, push in reverse order
                blocks.push_back(bb);

                // Get terminator
                const Instruction *terminator = bb->GetTerminator();

                // Handle terminator
                switch (terminator->opCode) {
                    default: {
                        // ASSERT(false, "Unknown terminator");
                        break;
                    }
                    case OpCode::Branch: {
                        auto *instr = terminator->As<BranchInstruction>();

                        // Push target branch
                        if (BasicBlock* branchBB = basicBlocks.GetBlock(instr->branch); Acquire(branchBB)) {
                            frontTraversal.push(branchBB);
                        }
                        break;
                    }
                    case OpCode::BranchConditional: {
                        auto *instr = terminator->As<BranchConditionalInstruction>();

                        // Push pass branch
                        if (BasicBlock* branchBB = basicBlocks.GetBlock(instr->pass); Acquire(branchBB)) {
                            frontTraversal.push(branchBB);
                        }
                        
                        // Push fail branch
                        if (BasicBlock* branchBB = basicBlocks.GetBlock(instr->fail); Acquire(branchBB)) {
                            frontTraversal.push(branchBB);
                        }
                        break;
                    }
                    case OpCode::Switch: {
                        auto *instr = terminator->As<SwitchInstruction>();

                        // Push default branch
                        if (BasicBlock* branchBB = basicBlocks.GetBlock(instr->_default); Acquire(branchBB)) {
                            frontTraversal.push(branchBB);
                        }
                        
                        // Push case branches
                        for (uint32_t caseIndex = 0; caseIndex < instr->cases.count; caseIndex++) {
                            if (BasicBlock* branchBB = basicBlocks.GetBlock(instr->cases[caseIndex].branch); Acquire(branchBB)) {
                                frontTraversal.push(branchBB);
                            }
                        }
                        break;
                    }
                    case OpCode::Return: {
                        break;
                    }
                }
            }

            // Blocks are pushed in LIFO, reverse it
            std::ranges::reverse(blocks);
        }

        /// Attempt to acquire a basic block
        /// \param bb basic block
        /// \return true if acquired
        bool Acquire(BasicBlock* bb) {
            const uint32_t index = bb->GetID() / 32;
            const uint32_t bit   = 1u << (bb->GetID() % 32);

            // Already visited?
            if (visitedStates[index] & bit) {
                return false;
            }

            // Mark as visited
            visitedStates[index] |= bit;
            return true;
        }

        /// Clear the state
        /// \param basicBlocks all basic blocks
        void Clear(BasicBlockList& basicBlocks) {
            // Cleanup
            visitedStates.clear();
            blocks.clear();

            // Determine the effective bound
            uint32_t bound = 0;
            for (BasicBlock* bb : basicBlocks) {
                bound = std::max(bound, bb->GetID() + 1u);
            }

            // Set number of visitation states
            visitedStates.resize((bound + 31) / 32);
        }

    private:
        /// All visitation states
        std::vector<uint32_t> visitedStates;

        /// All blocks
        std::vector<BasicBlock*> blocks;
    };
}