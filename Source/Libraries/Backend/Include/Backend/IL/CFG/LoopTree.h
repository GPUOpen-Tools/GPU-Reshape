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
#include "Common/Containers/TrivialStackVector.h"
#include <Backend/IL/CFG/DominatorTree.h>
#include <Backend/IL/CFG/Loop.h>

namespace IL {
    class LoopTree {
    public:
        using LoopView = std::vector<Loop>;

        /// Constructor
        /// \param dominatorTree domination tree from which to construct loop information
        LoopTree(const DominatorTree &dominatorTree) : dominatorTree(dominatorTree) {

        }

        /// Compute the loop view
        void Compute() {
            // Current back-edge stack
            std::vector<BasicBlock *> backEdgeBlocks;

            // Visit all blocks in post-order
            for (BasicBlock *header: dominatorTree.GetPostOrderTraversal().GetView()) {
                backEdgeBlocks.clear();

                // If the header dominates the predecessor, this is a back-edge
                for (BasicBlock *predecessor: dominatorTree.GetPredecessors(header)) {
                    if (dominatorTree.Dominates(header, predecessor)) {
                        backEdgeBlocks.push_back(predecessor);
                    }
                }

                // Any?
                if (backEdgeBlocks.empty()) {
                    continue;
                }

                // Construct loop information
                Loop& loop = loops.emplace_back();
                loop.header = header;
                loop.backEdgeBlocks.Resize(backEdgeBlocks.size());
                std::copy_n(backEdgeBlocks.begin(), backEdgeBlocks.size(), loop.backEdgeBlocks.begin());

                // Map out all inner-blocks
                MapBackEdgePredecessors(loop);

                // Map out all exits
                MapExitBlocks(loop);
            }
        }

        /// Get the loop view
        const LoopView& GetView() const {
            return loops;
        }

    private:
        /// Map all intra-loop blocks
        /// \param loop given loop
        void MapBackEdgePredecessors(Loop& loop) {
            TrivialStackVector<BasicBlock *, 32u> reverseWalkStack;

            // Cleanup
            ClearVisitationStates();

            // Append header
            AcquireVisitation(loop.header);
            loop.blocks.Add(loop.header);

            // Add all back edges
            for (BasicBlock *backEdge: loop.backEdgeBlocks) {
                AcquireVisitation(backEdge);
                reverseWalkStack.Add(backEdge);
            }

            // Walk the reverse stack until empty
            while (reverseWalkStack.Size()) {
                BasicBlock* bb = reverseWalkStack.PopBack();

                // Add as block
                loop.blocks.Add(bb);

                // Visit all predecessors
                for (BasicBlock* predecessor : dominatorTree.GetPredecessors(bb)) {
                    // Back at header?
                    if (!AcquireVisitation(predecessor)) {
                        continue;
                    }

                    // Visit predecessor
                    reverseWalkStack.Add(predecessor);
                }
            }
        }

        /// Map all exit blocks
        /// \param loop given loop
        void MapExitBlocks(Loop& loop) {
            // If a given successor of a known-mapped block is *not* known, this is an exit block
            for (BasicBlock* bb : loop.blocks) {
                for (BasicBlock* successor : dominatorTree.GetSuccessors(bb)) {
                    if (!IsAcquired(successor)) {
                        loop.exitBlocks.Add(successor);
                    }
                }
            }
        }

        /// Clear current vistation states
        void ClearVisitationStates() {
            // Cleanup
            visitedStates.clear();

            // Determine the effective bound
            uint32_t bound = 0;
            for (BasicBlock* bb : dominatorTree.GetBasicBlocks()) {
                bound = std::max(bound, bb->GetID() + 1u);
            }

            // Set number of visitation states
            visitedStates.resize((bound + 31) / 32);
        }

        /// Check if a basic block has been acquired
        /// \param bb basic block
        /// \return true if acquired
        bool IsAcquired(BasicBlock* bb) {
            const uint32_t index = bb->GetID() / 32;
            const uint32_t bit   = 1u << (bb->GetID() % 32);

            // Already visited?
            return visitedStates[index] & bit;
        }

        /// Attempt to acquire a basic block
        /// \param bb basic block
        /// \return true if acquired
        bool AcquireVisitation(BasicBlock* bb) {
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

    private:
        /// All loops
        LoopView loops;

        /// All visitation states
        std::vector<uint32_t> visitedStates;

        /// Domination tree
        const DominatorTree& dominatorTree;
    };
}
