#pragma once

// Backend
#include <Backend/IL/Function.h>

// Std
#include <variant>
#include <vector>

namespace IL {
    class BasicBlockTraversal {
    public:
        using BlockView = std::vector<BasicBlock *>;

        /// Perform post-order traversal
        /// \param function given function to traverse
        void PostOrder(Function *function) {
            Clear(function);

            // Invoke traversal
            TraversePostOrder(function, function->GetBasicBlocks().GetEntryPoint());
        }

        /// Get the current traversal view
        const BlockView &GetView() const {
            return blocks;
        }

    private:
        /// Perform post-order traversal
        /// \param function given function to traverse
        /// \param bb basic block to traverse
        void TraversePostOrder(Function* function, BasicBlock *bb) {
            // Attempt to acquire the basic block
            if (!Acquire(bb)) {
                return;
            }

            // All basic blocks
            IL::BasicBlockList &basicBlocks = function->GetBasicBlocks();

            // Get terminator
            const Instruction *terminator = bb->GetTerminator();

            // Handle terminator
            switch (terminator->opCode) {
                default:
                ASSERT(false, "Unknown terminator");
                    break;
                case OpCode::Branch: {
                    auto *instr = terminator->As<BranchInstruction>();
                    TraversePostOrder(function, basicBlocks.GetBlock(instr->branch));
                    break;
                }
                case OpCode::BranchConditional: {
                    auto *instr = terminator->As<BranchConditionalInstruction>();
                    TraversePostOrder(function, basicBlocks.GetBlock(instr->pass));
                    TraversePostOrder(function, basicBlocks.GetBlock(instr->fail));
                    break;
                }
                case OpCode::Return: {
                    break;
                }
            }

            // Mark as candidate
            blocks.push_back(bb);
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
        /// \param function function to clear to
        void Clear(Function* function) {
            // Cleanup
            visitedStates.clear();
            blocks.clear();

            // Determine the effective bound
            uint32_t bound = 0;
            for (BasicBlock* bb : function->GetBasicBlocks()) {
                bound = std::max(bound, bb->GetID());
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