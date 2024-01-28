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
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/Instruction.h>
#include <Backend/IL/BasicBlockList.h>
#include <Backend/IL/Analysis/CFG/BasicBlockTraversal.h>

// Std
#include <vector>
#include <unordered_map>

namespace IL {
    class DominatorAnalysis {
    public:
        using BlockView = std::vector<BasicBlock*>;

        /// Constructor
        /// \param basicBlocks all basic blocks
        DominatorAnalysis(BasicBlockList& basicBlocks) : basicBlocks(basicBlocks) {

        }

        /// Compute the dominator tree
        void Compute() {
            /** Loosely based on on https://www.cs.rice.edu/~keith/Embed/dom.pdf */

            // Initialize all blocks
            InitializeBlocks();

            // Get entry point
            IL::BasicBlock* entryPoint = basicBlocks.GetEntryPoint();

            // Mark the immediate entry point dominator
            GetBlock(entryPoint).immediateDominator = entryPoint;

            // Map out all blocks
            MapBlocks();

            // Mutation loop
            for (;;) {
                bool mutated = false;

                // Post order
                for (BasicBlock* bb : poTraversal.GetView()) {
                    if (bb == entryPoint) {
                        continue;
                    }

                    // Current immediate dominator
                    BasicBlock* immediateDomiator{nullptr};

                    // Consider all predecessors as candidates
                    for (BasicBlock* predecessor : GetBlock(bb).predecessors) {
                        // Ignore if the predecessor hasn't computed the immediate dominator
                        if (!GetBlock(predecessor).immediateDominator) {
                            continue;
                        }

                        // Assign dominator
                        if (!immediateDomiator) {
                            immediateDomiator = predecessor;
                        } else {
                            BasicBlock* first = immediateDomiator;
                            BasicBlock* second = predecessor;

                            // Algorithm finger1/2 magic
                            while (first != second) {
                                while (GetBlock(first).orderIndex < GetBlock(second).orderIndex) {
                                    first = GetBlock(first).immediateDominator;
                                }

                                while (GetBlock(second).orderIndex < GetBlock(first).orderIndex) {
                                    second = GetBlock(second).immediateDominator;
                                }
                            }

                            immediateDomiator = first;
                        }
                    }

                    // Assign if different
                    if (Block& block = GetBlock(bb); block.immediateDominator != immediateDomiator) {
                        block.immediateDominator = immediateDomiator;
                        mutated = true;
                    }
                }

                // Done?
                if (!mutated) {
                    break;
                }
            }
        }

        /// Determine if a basic block dominates another
        /// \param first domainating block
        /// \param second block being dominated
        /// \return true if first dominates second
        bool Dominates(const BasicBlock* first, const BasicBlock* second) const {
            BasicBlock* entryPoint = basicBlocks.GetEntryPoint();
            if (first == entryPoint) {
                return true;
            }

            // First dominator
            BasicBlock* immediateDominator = GetImmediateDominator(second);

            // Walk backwards
            while (immediateDominator != first && immediateDominator != entryPoint) {
                immediateDominator = GetImmediateDominator(immediateDominator);
            }

            // Found?
            return immediateDominator == first;
        }

        /// Get the immediate dominator of a basic block
        /// \param bb basic block
        /// \return immediate dominator
        BasicBlock* GetImmediateDominator(const BasicBlock* bb) const {
            return blocks.at(bb->GetID()).immediateDominator;
        }

        /// Get the predecessors of a basic block
        /// \param bb basic block
        /// \return predecessors
        const BlockView& GetPredecessors(const BasicBlock* bb) const {
            return blocks.at(bb->GetID()).predecessors;
        }

        /// Get the successprs of a basic block
        /// \param bb basic block
        /// \return successors
        const BlockView& GetSuccessors(const BasicBlock* bb) const {
            return blocks.at(bb->GetID()).successors;
        }

        /// Get the post order traversal
        /// \return traversal
        const BasicBlockTraversal& GetPostOrderTraversal() const {
            return poTraversal;
        }

        /// Get a block
        /// \param id block identifier
        /// \return nullptr if not found
        BasicBlock* GetBlock(IL::ID id) const {
            auto it = blocks.find(id);
            return it != blocks.end() ? it->second.basicBlock : nullptr;
        }
        
        /// Get all basic blocks
        BasicBlockList& GetBasicBlocks() const {
            return basicBlocks;
        }

    private:
        struct Block {
            /// Underlying basic block
            BasicBlock* basicBlock{nullptr};
            
            /// Current immediate dominator
            BasicBlock* immediateDominator{nullptr};

            /// All predecessors
            BlockView predecessors;

            /// All successors
            BlockView successors;

            /// Ordering index
            uint32_t orderIndex{0};
        };

    private:
        /// Get the block
        Block& GetBlock(BasicBlock* bb) {
            return blocks[bb->GetID()];
        }

        /// Initialize all blocks
        void InitializeBlocks() {
            blocks.clear();

            // Reset block states
            for (BasicBlock* bb : basicBlocks) {
                Block& block = blocks[bb->GetID()];
                block.basicBlock = bb;
                block.immediateDominator = nullptr;
                block.predecessors.clear();
                block.orderIndex = 0;
            }
        }

        /// Map out all blocks
        void MapBlocks() {
            // Perform post-order traversal
            poTraversal.PostOrder(basicBlocks);

            // Get final order
            const BasicBlockTraversal::BlockView& view = poTraversal.GetView();

            // Visit in post-order
            for (size_t i = 0; i < view.size(); i++) {
                BasicBlock* bb = view[i];

                // Assign order index, used for finger comparison
                blocks[bb->GetID()].orderIndex = static_cast<uint32_t>(i) + 1;

                // Get the terminator
                const Instruction* terminator = bb->GetTerminator();

                // Handle terminator
                switch (terminator->opCode) {
                    default:
                        // ASSERT(false, "Unknown terminator");
                        break;
                    case OpCode::Branch: {
                        auto* instr = terminator->As<BranchInstruction>();
                        AddPredecessor(basicBlocks.GetBlock(instr->branch), bb);
                        break;
                    }
                    case OpCode::BranchConditional: {
                        auto* instr = terminator->As<BranchConditionalInstruction>();
                        AddPredecessor(basicBlocks.GetBlock(instr->pass), bb);
                        AddPredecessor(basicBlocks.GetBlock(instr->fail), bb);
                        break;
                    }
                    case OpCode::Switch: {
                        auto* instr = terminator->As<SwitchInstruction>();
                        AddPredecessor(basicBlocks.GetBlock(instr->_default), bb);
                        for (uint32_t caseIndex = 0; caseIndex < instr->cases.count; caseIndex++) {
                            AddPredecessor(basicBlocks.GetBlock(instr->cases[caseIndex].branch), bb);
                        }
                        break;
                    }
                    case OpCode::Return: {
                        break;
                    }
                }
            }
        }

        /// Add a block predecessor
        /// \param block destination block
        /// \param from given predecessor
        void AddPredecessor(BasicBlock* block, BasicBlock* from) {
            blocks[block->GetID()].predecessors.push_back(from);
            blocks[from->GetID()].successors.push_back(block);
        }

    private:
        /// All blocks
        std::unordered_map<IL::ID, Block> blocks;

    private:
        /// Source basic blocks
        BasicBlockList& basicBlocks;

        /// Post-order traversal
        BasicBlockTraversal poTraversal;
    };
}