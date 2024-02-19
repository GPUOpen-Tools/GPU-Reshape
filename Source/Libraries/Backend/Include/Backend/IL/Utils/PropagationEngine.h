#pragma once

// Backend
#include <Backend/IL/InstructionCommon.h>
#include <Backend/IL/Analysis/UserAnalysis.h>
#include <Backend/IL/Analysis/CFG/DominatorAnalysis.h>
#include <Backend/IL/Analysis/CFG/LoopAnalysis.h>
#include <Backend/IL/Utils/PropagationResult.h>

// Std
#include <set>
#include <vector>
#include <queue>

namespace Backend::IL {
    class PropagationEngine {
    public:
        /** Loosely based on
         *  https://dl.acm.org/doi/10.1145/103135.103136
         *  https://www.researchgate.net/publication/255650058_A_Propagation_Engine_for_GCC
         *
         *  And inspired by
         *  https://www.researchgate.net/publication/221477318_Enabling_Sparse_Constant_Propagation_of_Array_Elements_via_Array_SSA_Form
         **/

        PropagationEngine(Program& program, Function& function) :
            program(program), function(function) {
        }

        /// Compute propagation across the function
        /// \param context propagation context
        template<typename F>
        bool Compute(F& context) {
            // Compute instruction user analysis to ssa-edges
            if (userAnalysis = program.GetAnalysisMap().FindPassOrCompute<UserAnalysis>(program); !userAnalysis) {
                return false;
            }

            // Compute dominator analysis for propagation
            if (dominatorAnalysis = function.GetAnalysisMap().FindPassOrCompute<DominatorAnalysis>(function); !dominatorAnalysis) {
                return false;
            }

            // Compute loop analysis for simulation
            if (loopAnalysis = function.GetAnalysisMap().FindPassOrCompute<LoopAnalysis>(function); !loopAnalysis) {
                return false;
            }
            
            // Initialize loop headers
            for (const Loop& loop : loopAnalysis->GetView()) {
#ifdef _MSC_VER // Compiler bug workaround
                LoopHeaderItem& item = loopHeaders[loop.header];
                item.definition = &loop;
#else // _MSC_VER
                loopHeaders[loop.header] = LoopHeaderItem {
                    .definition = &loop
                };
#endif // _MSC_VER
            }

            // Seed to entry point
            WorkItem work;
            SeedCFGEdge(work, nullptr, function.GetBasicBlocks().GetEntryPoint(), PropagationResult::Mapped);

            // Propagate all values
            Propagate(work, context);

            // OK
            return true;
        }

        /// Check if an edge is marked as executable
        /// \param from source basic block
        /// \param to destination basic block
        /// \return true if executable
        bool IsEdgeExecutable(const BasicBlock* from, const BasicBlock* to) {
            if (!IsExecutableLoopEdge(*workContext, from, to)) {
                return false;
            }

            return cfgExecutableEdges.contains(Edge {
                .from = from,
                .to = to
            });
        }

    private:
        struct LoopHeaderItem {
            /// Source definition
            const Loop* definition{nullptr};
        };

        struct Edge {
            /// Edge points
            const BasicBlock* from{nullptr};
            const BasicBlock* to{nullptr};

            /// Propagated control-flow lattice
            PropagationResult lattice{PropagationResult::None};

            /// Sorting does not check lattice
            bool operator<(const Edge& key) const {
                return std::make_tuple(from, to) < std::make_tuple(key.from, key.to);
            }

            /// Equality does not check lattice
            bool operator==(const Edge& rhs) const {
                return from == rhs.from && to == rhs.to;
            }
        };

        struct LoopItem {
            /// Header of this item
            const LoopHeaderItem* header{nullptr};

            /// Has any exit edges been hit?
            bool anyExitEdges{false};

            /// All escaping paths
            std::vector<Edge> latchAndExitEdges;

            /// All executed edges
            std::vector<Edge> edges;

            /// Current number of iterations
            uint32_t iterationCount{0};
        };

        struct SSAItem {
            /// Edge that requested this SSA item
            Edge edge;

            /// Instruction to be executed
            const Instruction* instr{nullptr};
        };

        struct WorkItem {
            /// Queued CFG stack
            std::queue<Edge> cfgWorkStack;

            /// Queued SSA stack
            std::queue<SSAItem> ssaWorkStack;

            /// Optional, current loop being executed
            LoopItem* loop{nullptr};
        };

    private:
        /// Propagate all work in a work item
        /// \param work work item
        /// \param context propagation context
        template<typename F>
        void Propagate(WorkItem& work, F& context) {
            for (;;) {
                bool any = false;

                // Work through pending CFGs
                if (!work.cfgWorkStack.empty()) {
                    // Special case, if the CFG item is a loop header, spawn a separate work item
                    if (auto loopItem = loopHeaders.find(work.cfgWorkStack.front().to); loopItem != loopHeaders.end()) {
                        PropagateLoopHeader(work, context, loopItem->second);
                    } else {
                        PropagateCFG(work, context);
                    }

                    any = true;
                }

                // Work through pending SSAs
                if (!work.ssaWorkStack.empty()) {
                    PropagateSSA(work, context);
                    any = true;
                }

                // No work? Exit!
                if (!any) {
                    break;
                }
            }
        }

        /// Propagate all constants in a loop complex
        /// \param outerWork the outer work stack, front most item must be loop header
        /// \param context propagation context
        /// \param headerItem found header item to execute
        template<typename F>
        void PropagateLoopHeader(WorkItem& outerWork, F& context, LoopHeaderItem& headerItem) {
            Edge edge = outerWork.cfgWorkStack.front();
            outerWork.cfgWorkStack.pop();

            // Current incoming edge, changed on the next iteration
            Edge incomingEdge = edge;

            // Persistent loop info
            LoopItem loopItem;
            loopItem.header = &headerItem;

            for (;;) {
                // Has a termination condition been met?
                bool terminationOrVarying = false;

                // Setup work item
                WorkItem loopWork;
                loopWork.loop = &loopItem;

                // If the incoming is not that of the outer work item, mark it as executed
                // This is to satisfy Phi constraints.
                if (incomingEdge != edge) {
                    cfgExecutableEdges.insert(incomingEdge);
                }

                // Evaluate header manually,
                // ensures that the loop guard doesn't catch the header
                loopWork.cfgWorkStack.push(incomingEdge);
                PropagateCFG(loopWork, context);

                // If the incoming is not that of the outer work item, remove it as executed
                // This is to satisfy Phi constraints.
                if (incomingEdge != edge) {
                    cfgExecutableEdges.erase(incomingEdge);
                }

                // Run propagation for this iteration
                Propagate(loopWork, context);
                loopItem.iterationCount++;

                // Propagate all side effects caused by the loop
                context.PropagateLoopEffects(headerItem.definition);

                // If any exit edges have been met, terminate execution entirely
                // This is a surprisingly complicated topic, and something I've been thinking about for a while
                // I need to track variability throughout the internal control flow, however, that's not as simple
                // as just attributing varying'ness to each edge, since it may merge later. And a bunch of other
                // things. It's likely a solved problem by people much smarter than myself, I just need to find
                // the literature somewhere.
                if (loopItem.anyExitEdges) {
                    terminationOrVarying = true;
                }

                // Loops may have multiple edges active, re-iteration
                // is dependent on that there's a single mapped, i.e. non varying,
                // edge to the header again.
                bool hasNonVaryingReEnrant = false;
                for (auto && tag : loopItem.latchAndExitEdges) {
                    if (/*tag.lattice == PropagationResult::Mapped &&*/ tag.to == headerItem.definition->header) {
                        hasNonVaryingReEnrant = true;
                        incomingEdge = tag;
                    }
                }

                // If there's no known re-entry, stop
                bool knownExitEdge = false;
                if (!hasNonVaryingReEnrant) {
                    for (auto && tag : loopItem.latchAndExitEdges) {
                        if (tag.to != headerItem.definition->header) {
                            outerWork.cfgWorkStack.push(tag);
                            knownExitEdge = true;
                        }
                    }

                    // Terminate!
                    terminationOrVarying = true;
                }

                // Stop propagation?
                if (terminationOrVarying) {
                    // If there's no known exits, add them all (could have been varying)
                    if (!knownExitEdge) {
                        for (const BasicBlock* exit : loopItem.header->definition->exitBlocks) {
                            outerWork.cfgWorkStack.push(Edge {
                                .from = loopItem.header->definition->header,
                                .to = exit
                            });
                        }
                    }
                    break;
                }

                /* Another cycle is set to begin, clean up the old cycle */

                loopItem.latchAndExitEdges.clear();
                loopItem.anyExitEdges = false;

                // Remove body from the set of executed blocks
                for (BasicBlock *block : loopItem.header->definition->blocks) {
                    executedBlocks.erase(block);

                    // Cleanup instructions
                    for (const Instruction* instr : *block) {
                        ssaLattice.erase(instr);
                        ssaExclusion.erase(instr);
                        context.ClearInstruction(instr);
                    }
                }

                // Remove all executed edges
                for (const Edge& edge_ : loopItem.edges) {
                    cfgExecutableEdges.erase(edge_);
                }
                
                loopItem.edges.clear();
            }
        }

        /// Propagate a CFG item
        /// \param work item to propagate
        /// \param context propagation context
        template<typename F>
        void PropagateCFG(WorkItem& work, F& context) {
            Edge edge = work.cfgWorkStack.front();
            work.cfgWorkStack.pop();

            // Certain instructions must always be propagated
            // Phi instructions depend on the incoming edge
            for (const Instruction* instr : *edge.to) {
                switch (instr->opCode) {
                    default: {
                        break;
                    }
                    case OpCode::Phi: {
                        PropagateSSA(work, edge, instr, context);
                        break;
                    }
                }
            }

            // Block are only simulated once, ssa statements have a separate work queue
            if (executedBlocks.contains(edge.to)) {
                return;
            }

            // Propagate all non-phi instructions
            for (const Instruction* instr : *edge.to) {
                if (instr->Is<PhiInstruction>()) {
                    continue;
                }

                PropagateSSA(work, edge, instr, context);
            }

            // Mark the block as executed
            executedBlocks.insert(edge.to);
        }

        /// Propagate an SSA item
        /// \param work item to propagate
        /// \param context propagation context
        template<typename F>
        void PropagateSSA(WorkItem& work, F& context) {
            SSAItem item = work.ssaWorkStack.front();
            work.ssaWorkStack.pop();

            PropagateSSA(work, item.edge, item.instr, context);
        }

        /// Propagate an SSA item
        /// \param work item to propagate
        /// \param context propagation context
        template<typename F>
        void PropagateSSA(WorkItem& work, const Edge& edge, const Instruction* instr, F& context) {
            // Instructions may be marked for exclusion
            if (ssaExclusion.contains(instr)) {
                return;
            }

            // TODO: This is a hack, maybe supply a context to the visitor?
            workContext = &work;

            // Get the current lattice result, may be None
            PropagationResult latticeResult = ssaLattice[instr];

            // Invoke visitor
            const BasicBlock* conditionalBlock = nullptr;
            PropagationResult propagationResult = context.PropagateInstruction(edge.to, instr, &conditionalBlock);

            // Cleanup of hack above
            workContext = nullptr;

            // Validate and set result
            ASSERT(!ssaLattice.contains(instr) || static_cast<uint32_t>(latticeResult) <= static_cast<uint32_t>(propagationResult), "Malformed lattice transition");
            ssaLattice[instr] = propagationResult;

            // Handle result
            switch (propagationResult) {
                default: {
                    ASSERT(false, "Invalid result");
                    return;
                }
                case PropagationResult::Ignore:
                case PropagationResult::Overdefined: {
                    PropagateNonVaryingOperands(work, edge.to, instr);
                    break;
                }
                case PropagationResult::Mapped: {
                    // TODO: Should be guarded
                    //if (propagationResult != latticeResult) {
                        SeedSSAEdges(work, edge, instr);
                    //}

                    if (conditionalBlock) {
                        SeedCFGEdge(work, edge.to, conditionalBlock, JoinEdgeLattice(edge.lattice, propagationResult));
                    }

                    PropagateNonVaryingOperands(work, edge.to, instr);
                    break;
                }
                case PropagationResult::Varying: {
                    ssaExclusion.insert(instr);

                    // TODO: Should be guarded
                    //if (propagationResult != latticeResult) {
                        SeedSSAEdges(work, edge, instr);
                    //}

                    // Varying terminators add all the successors to the work list
                    if (IsTerminator(instr)) {
                        for (const BasicBlock* successor : dominatorAnalysis->GetSuccessors(edge.to)) {
                            SeedCFGEdge(work, edge.to, successor, PropagationResult::Varying);
                        }
                    }
                    break;
                }
            }
        }

        /// Join two lattice values for edges
        PropagationResult JoinEdgeLattice(PropagationResult a, PropagationResult b) {
            return static_cast<PropagationResult>(std::max(static_cast<uint32_t>(a), static_cast<uint32_t>(b)));
        }

        /// Check if a block is an active loop back edge
        bool IsActiveLoopBackEdge(WorkItem& work, const BasicBlock* block) {
            if (!work.loop) {
                return false;
            }

            // Check all back edge blocks
            for (BasicBlock* backEdge : work.loop->header->definition->backEdgeBlocks) {
                if (block == backEdge) {
                    return true;
                }
            }

            return false;
        }

        /// Check if an edge is executable within a loop complex, useful for Phi resolving
        bool IsExecutableLoopEdge(WorkItem& work, const BasicBlock* from, const BasicBlock* to) {
            // Only concerned with branching to loop headers
            if (!work.loop || to != work.loop->header->definition->header) {
                return true;
            }

            // First entrants have no special considerations
            if (!work.loop->iterationCount) {
                return true;
            }

            // Loop re-entrants must only concern themselves with active back edges
            return IsActiveLoopBackEdge(work, from);
        }

        /// Propagate non-varying status of all operands
        void PropagateNonVaryingOperands(WorkItem& work, const BasicBlock* block, const Instruction* instr) {
            bool any = false;

            // Phi's are handled separately
            if (auto* phi = instr->Cast<PhiInstruction>()) {
                for (uint32_t i = 0; i < phi->values.count; i++) {
                    PhiValue value = phi->values[i];

                    InstructionRef<> ref(program.GetIdentifierMap().Get(value.value));

                    Edge phiEdge {
                        .from = program.GetIdentifierMap().GetBasicBlock(value.branch),
                        .to = block
                    };

                    // If an active back edge, ignore
                    if (IsActiveLoopBackEdge(work, phiEdge.from)) {
                        continue;
                    }

                    // Are there any dependencies to it?
                    if (!cfgExecutableEdges.contains(phiEdge) || (ref && !ssaExclusion.contains(ref.Get()))) {
                        any = true;
                        break;
                    }
                }
            } else {
                VisitOperands(instr, [&](IL::ID id) {
                    if (auto ref = InstructionRef<>(program.GetIdentifierMap().Get(id))) {
                        any |= !ssaExclusion.contains(ref.Get());
                    }
                });
            }

            if (!any) {
                ssaExclusion.insert(instr);
            }
        }

        /// Seed all SSA users for a given instruction
        void SeedSSAEdges(WorkItem& work, const Edge& edge, const Instruction* instr) {
            if (instr->result == InvalidID) {
                return;
            }

            // Seed from all users
            for (ConstInstructionRef<> ref : userAnalysis->GetUsers(instr->result)) {
                const Instruction* userInstr = ref.Get();
                if (!userInstr) {
                    continue;
                }

                // If this block has not been executed, it will be later (if not, it's a dead block), do not seed
                if (!executedBlocks.contains(ref.basicBlock)) {
                    continue;
                }

                // Early check for exclusion
                if (ssaExclusion.contains(userInstr)) {
                    continue;
                }

                // Add to stack
                work.ssaWorkStack.push(SSAItem {
                    .edge = Edge {
                        .from = nullptr,
                        .to = ref.basicBlock,
                        .lattice = edge.lattice
                    },
                    .instr = userInstr
                });
            }
        }

        /// Seed a specific CFG edge
        void SeedCFGEdge(WorkItem& work, const BasicBlock* from, const BasicBlock* to, PropagationResult lattice) {
            if (to == nullptr) {
                return;
            }

            const Edge edge{
                .from = from,
                .to = to,
                .lattice = lattice
            };

            if (cfgExecutableEdges.contains(edge)) {
                // TODO: This needs guarding, but validate
                //return;
            }

            cfgExecutableEdges.insert(edge);

            // Active loops require special consideration
            if (work.loop) {
                bool isLatchOrExit = false;

                // Keep track of edge
                work.loop->edges.push_back(edge);

                // If branching back to the header, terminate
                isLatchOrExit |= to == work.loop->header->definition->header;

                // If branching to an exit, terminate
                for (BasicBlock *exit : work.loop->header->definition->exitBlocks) {
                    if (to == exit) {
                        work.loop->anyExitEdges = true;
                        isLatchOrExit = true;
                    }
                }

                // If terminating, record what terminated
                if (isLatchOrExit) {
                    work.loop->latchAndExitEdges.emplace_back(edge);
                    return;
                }
            }

            // Add to work stack
            work.cfgWorkStack.push(edge);
        }

    private:
        /// Outer program
        Program& program;

        /// Source function
        Function& function;

        /// Dominator analysis
        ComRef<DominatorAnalysis> dominatorAnalysis;

        /// Loop analysis
        ComRef<LoopAnalysis> loopAnalysis;

        /// User analysis
        ComRef<UserAnalysis> userAnalysis;

        /// Current work context, this is a hack
        WorkItem* workContext{nullptr};

    private:
        /// All known executable edges
        std::set<Edge> cfgExecutableEdges;

        /// All executed blocks
        std::set<const BasicBlock*> executedBlocks;

    private:
        /// All instructions excluded from propagation
        std::set<const Instruction*> ssaExclusion;

        /// All lattice mappings
        std::map<const Instruction*, PropagationResult> ssaLattice;

    private:
        /// All mapped loop headers
        std::map<const BasicBlock*, LoopHeaderItem> loopHeaders;
    };
}
