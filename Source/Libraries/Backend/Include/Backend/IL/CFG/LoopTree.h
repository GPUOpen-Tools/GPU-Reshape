#pragma once

// Backend
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
            std::vector<BasicBlock *> backEdges;

            // Visit all blocks in post-order
            for (BasicBlock *header: dominatorTree.GetPostOrderTraversal().GetView()) {
                backEdges.clear();

                // If the header dominates the predecessor, this is a back-edge
                for (BasicBlock *predecessor: dominatorTree.GetPredecessors(header)) {
                    if (dominatorTree.Dominates(header, predecessor)) {
                        backEdges.push_back(predecessor);
                    }
                }

                // Any?
                if (backEdges.empty()) {
                    continue;
                }

                // Construct loop information
                Loop& loop = loops.emplace_back();
                loop.header = header;
                loop.backEdges.Resize(backEdges.size());
                std::copy_n(backEdges.begin(), backEdges.size(), loop.backEdges.begin());
            }
        }

        /// Get the loop view
        const LoopView& GetView() const {
            return loops;
        }

    private:
        /// All loops
        LoopView loops;

        /// Domination tree
        const DominatorTree& dominatorTree;
    };
}
