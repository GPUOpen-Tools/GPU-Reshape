#pragma once

// Backend
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/Instruction.h>
#include <Backend/IL/BasicBlockList.h>
#include <Backend/IL/CFG/BasicBlockTraversal.h>

// Std
#include <vector>
#include <unordered_map>

namespace IL {
    class DominatorTree {
    public:
        using BlockView = std::vector<BasicBlock*>;

        /// Constructor
        /// \param basicBlocks all basic blocks
        DominatorTree(BasicBlockList& basicBlocks) : basicBlocks(basicBlocks) {

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
        bool Dominates(BasicBlock* first, BasicBlock* second) const {
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
        BasicBlock* GetImmediateDominator(BasicBlock* bb) const {
            return blocks.at(bb->GetID()).immediateDominator;
        }

        /// Get the predecessors of a basic block
        /// \param bb basic block
        /// \return predecessors
        const BlockView& GetPredecessors(BasicBlock* bb) const {
            return blocks.at(bb->GetID()).predecessors;
        }

        /// Get the successprs of a basic block
        /// \param bb basic block
        /// \return successors
        const BlockView& GetSuccessors(BasicBlock* bb) const {
            return blocks.at(bb->GetID()).successors;
        }

        /// Get the post order traversal
        /// \return traversal
        const BasicBlockTraversal& GetPostOrderTraversal() const {
            return poTraversal;
        }

        /// Get all basic blocks
        BasicBlockList& GetBasicBlocks() const {
            return basicBlocks;
        }

    private:
        struct Block {
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