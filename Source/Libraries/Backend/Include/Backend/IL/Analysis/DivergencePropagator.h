//
// The MIT License (MIT)
//
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
#include <Backend/IL/Analysis/ISimulationPropagator.h>
#include <Backend/IL/Analysis/ConstantPropagator.h>
#include <Backend/IL/Analysis/WorkGroupDivergence.h>
#include <Backend/IL/Utils/PropagationEngine.h>

namespace IL {
    class DivergencePropagator : public ISimulationPropagator {
    public:
        COMPONENT(DivergencePropagator);

        /// Constructor
        /// \param program program to propagate divergence for
        /// \param function function to propagate divergence for
        DivergencePropagator(const ConstantPropagator& constants, Program& program, Function& function) : constantPropagator(constants), program(program), function(function) {

        }

        /// Get the divergence of a value
        WorkGroupDivergence GetDivergence(ID id) const {
            return divergenceValues.at(id).divergence;
        }

    public:
        /// Install this propagator
        bool Install(Backend::IL::PropagationEngine* engine) override {
            propagationEngine = engine;
            
            divergenceValues.resize(program.GetIdentifierMap().GetMaxID(), DivergenceState {
                .divergence = WorkGroupDivergence::Unknown
            });

            // Compute dominator analysis for propagation
            if (dominatorAnalysis = function.GetAnalysisMap().FindPassOrCompute<DominatorAnalysis>(function); !dominatorAnalysis) {
                return false;
            }

            // Compute loop analysis for simulation
            if (loopAnalysis = function.GetAnalysisMap().FindPassOrCompute<LoopAnalysis>(function); !loopAnalysis) {
                return false;
            }

            // Map loop views
            for (const Loop& loop : loopAnalysis->GetView()) {
                loopLookup[loop.header] = &loop;
            }
            
            // Mark constants as uniform
            for (const Constant *constant: program.GetConstants()) {
                if (constant->IsSymbolic()) {
                    continue;
                }

                MarkAsUniform(constant->id);
            }

            // Conditionally mark variables as divergent
            for (const Backend::IL::Variable *variable: program.GetVariableList()) {
                if (variable->initializer) {
                    MarkAsUniform(variable->id);
                    continue;
                }

                // Otherwise, assume from AS
                SetDivergence(variable->id, GetGlobalAddressSpaceDivergence(variable->addressSpace));
            }

            // Mark all function inputs as divergent
            for (const Backend::IL::Variable *variable: function.GetParameters()) {
                MarkAsDivergent(variable->id);
            }

            // OK
            return true;
        }

        /// Propagate an instruction
        void PropagateInstruction(Backend::IL::PropagationResult, const BasicBlock* block, const Instruction* instr, const BasicBlock*) override {
            switch (instr->opCode) {
                default: {
                    // Result-less instructions are ignored
                    if (instr->result == InvalidID) {
                        return;
                    }

                    PropagateResultInstruction(instr);
                    break;
                }
                case OpCode::AddressChain: {
                    break;
                }
                case OpCode::Load: {
                    PropagateLoadInstruction(block, instr->As<LoadInstruction>());
                    break;
                }
                case OpCode::Store: {
                    PropagateStoreInstruction(instr->As<StoreInstruction>());
                    break;
                }
                case OpCode::Phi: {
                    PropagatePhiInstruction(block, instr->As<PhiInstruction>());
                    break;
                }
            }
        }

        /// Propagate all loop side-effects
        void PropagateLoopEffects(const Loop* loop) override {
            /** poof */
        }
        
        /// Simulate a static store operation
        void StoreStatic(ID target, ID source) {
            
        }

    private:
        void PropagateResultInstruction(const Instruction *instr) {
            SetDivergence(instr->result, GetInstructionDivergence(instr));
        }

        /// Load instruction propagation
        void PropagateLoadInstruction(const BasicBlock* block, const LoadInstruction *instr) {
            // If the memory space is divergent, so is this
            if (GetBaseDivergence(instr) == WorkGroupDivergence::Divergent) {
                MarkAsDivergent(instr->result);
                return;
            }

            // Traverse the memory tree
            // Do not instantiate new nodes, assume from last written
            MemoryTreeTraversal memory = GetMemoryTreeNode(instr->address, false);
            if (!memory.node) {
                ASSERT(false, "Unexpected memory state");
                return;
            }

            // If the tree traversal is divergent, so is this
            if (memory.divergence == WorkGroupDivergence::Divergent) {
                MarkAsDivergent(instr->result);
                return;
            }

            // Find store candidates from memory pattern
            TrivialStackVector<ReachingCandidate, 8> candidates;
            FindReachingStoreDefinitions(block, instr, memory.node, candidates);
            
            TrivialStackVector<const BasicBlock*, 8> blocks;

            // Find all block candidates
            for (const ReachingCandidate& candidate : candidates) {
                // If any of the values are divergent, this store will be too
                if (candidate.stored.divergence == WorkGroupDivergence::Divergent) {
                    MarkAsDivergent(instr->result);
                    return;
                }

                // Local block always appended, don't double append
                if (candidate.block != block) {
                    blocks.Add(candidate.block);
                }
            }

            // Local block for intersection testing
            blocks.Add(block);

            // Check if the intersection between the idom paths are divergent
            if (GetBlockIntersectionDivergence(block, blocks) == WorkGroupDivergence::Divergent) {
                MarkAsDivergent(instr->result);
                return;
            }
            
            // Addresses themselves are never marked as divergent, unless they are variables, i.e. globals and parameters
            if (IsDivergent(instr->address)) {
                MarkAsDivergent(instr->result);
                return;
            }

            // Assume uniform!
            MarkAsUniform(instr->result);
        }

        /// Store instruction propagation
        void PropagateStoreInstruction(const StoreInstruction *instr) {
            // Find or instantiate the memory tree
            MemoryTreeTraversal memory = GetMemoryTreeNode(instr->address, true);
            if (!memory.node) {
                ASSERT(false, "Unexpected memory state");
                return;
            }

            // Divergence is stored on the lookup, not memory tree, since that's not unique
            ssaDivergenceLookup[instr] = StoredDivergence {
                .memory = memory.node,
                .divergence = GetDivergence(instr->value)
            };
        }

        /// Phi instruction propagation
        void PropagatePhiInstruction(const BasicBlock* block, const PhiInstruction *instr) {
            // Get all blocks
            TrivialStackVector<const BasicBlock*, 8> phiBlocks;
            
            for (uint32_t i = 0; i < instr->values.count; i++) {
                // If the phi value is divergent, then everything is
                if (IsDivergent(instr->values[i].value)) {
                    MarkAsDivergent(instr->result);
                    return;
                }
                
                phiBlocks.Add(program.GetIdentifierMap().GetBasicBlock(instr->values[i].branch));
            }

            // Assume idom path intersection divergence
            SetDivergence(instr->result, GetBlockIntersectionDivergence(block, phiBlocks));
        }

        /// Get the divergence of an instruction
        /// \param instr propagated instruction
        /// \return resulting divergence
        WorkGroupDivergence GetInstructionDivergence(const Instruction* instr) {
            // Instructions may converge divergent values
            if (IsConvergingInstruction(instr)) {
                return WorkGroupDivergence::Uniform;
            }

            // Determine the base divergence, that is, the divergence of the instruction type itself
            WorkGroupDivergence baseDivergence = GetBaseDivergence(instr);

            // If divergent, don't check operands
            if (baseDivergence == WorkGroupDivergence::Divergent) {
                return baseDivergence;
            }

            // Attributes
            bool isAnyUnknown = false;
            bool isAnyDivergent = false;

            // Collect operand attributes
            Backend::IL::VisitOperands(instr, [&](IL::ID id) {
                isAnyUnknown |= divergenceValues[id].divergence == WorkGroupDivergence::Unknown;
                isAnyDivergent |= divergenceValues[id].divergence == WorkGroupDivergence::Divergent;
            });

            // If any are divergent, presume divergent
            if (isAnyDivergent) {
                return WorkGroupDivergence::Divergent;
            }

            // If any are unknown, we cannot make any presumption
            if (isAnyUnknown) {
                return WorkGroupDivergence::Unknown;
            }

            // Assume uniform
            return WorkGroupDivergence::Uniform;
        }

        /// Check if an instruction is converging
        /// \param instr instruction to query
        /// \return convergence
        bool IsConvergingInstruction(const Instruction * instr) {
            switch (instr->opCode) {
                default: {
                    return false;
                }
                case OpCode::WaveReadFirst: {
                    return true;
                }
            }
        }

        /// Get the base divergence of an instruction
        /// This does not include operands or any carried / associated divergence
        /// \param instr instruction to query
        /// \return divergence
        WorkGroupDivergence GetBaseDivergence(const Instruction* instr) {
            switch (instr->opCode) {
                default: {
                    return WorkGroupDivergence::Uniform;
                }

                /** Assume divergence by backend traits */
                case OpCode::Unexposed: {
                    auto typed = instr->As<UnexposedInstruction>();
                    return AsDivergence(typed->traits.divergent);
                }

                /** Load operations to external memory are always divergent */
                case OpCode::Load: {
                    auto* type = program.GetTypeMap().GetType(instr->As<LoadInstruction>()->address)->As<Backend::IL::PointerType>();
                    return AsDivergence(
                        type->addressSpace == Backend::IL::AddressSpace::Buffer ||
                        type->addressSpace == Backend::IL::AddressSpace::Texture ||
                        type->addressSpace == Backend::IL::AddressSpace::Resource
                    );
                }

                /** Atomic operations are always divergent */
                case OpCode::AtomicOr:
                case OpCode::AtomicXOr:
                case OpCode::AtomicAnd:
                case OpCode::AtomicAdd:
                case OpCode::AtomicMin:
                case OpCode::AtomicMax:
                case OpCode::AtomicExchange:
                case OpCode::AtomicCompareExchange: {
                    return WorkGroupDivergence::Divergent;
                }

                /** Resource operations are always divergent */
                case OpCode::SampleTexture:
                case OpCode::LoadTexture:
                case OpCode::LoadBuffer: {
                    return WorkGroupDivergence::Divergent;
                }
            }
        }

        /// Get the divergence of a global address space
        WorkGroupDivergence GetGlobalAddressSpaceDivergence(Backend::IL::AddressSpace space) {
            switch (space) {
                default: {
                    ASSERT(false, "Not a global address space");
                    return WorkGroupDivergence::Uniform;
                }
                
                case Backend::IL::AddressSpace::Constant:
                case Backend::IL::AddressSpace::RootConstant: {
                    return WorkGroupDivergence::Uniform;
                }
                
                case Backend::IL::AddressSpace::Texture:
                case Backend::IL::AddressSpace::Buffer:
                case Backend::IL::AddressSpace::Resource:
                case Backend::IL::AddressSpace::GroupShared:
                case Backend::IL::AddressSpace::Function:
                case Backend::IL::AddressSpace::Input:
                case Backend::IL::AddressSpace::Output:
                case Backend::IL::AddressSpace::Unexposed: {
                    return WorkGroupDivergence::Divergent;
                }
            }
        }

    private:
        /// Get the divergence from a block set intersection
        /// \param block source block
        /// \param blocks all blocks to test
        /// \return divergence
        WorkGroupDivergence GetBlockIntersectionDivergence(const BasicBlock* block, std::span<const BasicBlock*> blocks) {
            const Loop* loop{nullptr};

            // Check if this block is a loop header
            if (auto it = loopLookup.find(block); it != loopLookup.end()) {
                loop = it->second;
            }
            
            TrivialStackVector<const BasicBlock*, 8> sharedAncestors;
            IntersectAllDominationPaths(loop, blocks, sharedAncestors);

            // Check divergence on all ancestors
            for (const BasicBlock* sharedAncestor : sharedAncestors) {
                auto terminator = sharedAncestor->GetTerminator().Get();

                // Conditional value
                ID value = InvalidID;

                // Get value from terminator
                switch (terminator->opCode) {
                    default: {
                        ASSERT(false, "Dominator ancestor terminator must be a branch");
                        continue;
                    }
                    case OpCode::Branch: {
                        // Irrelevant ancestor
                        continue;
                    }
                    case OpCode::BranchConditional: {
                        value = terminator->As<BranchConditionalInstruction>()->cond;
                        break;
                    }
                    case OpCode::Switch: {
                        value = terminator->As<SwitchInstruction>()->value;
                        break;
                    }
                }

                // If the branch condition is divergent, the phi is source of divergence
                if (IsDivergent(value)) {
                    return WorkGroupDivergence::Divergent;
                }
            } 

            // All paths are uniform
            return WorkGroupDivergence::Uniform;
        }

    private:
        struct BlockRange {
            size_t begin{0};
            size_t end{0};
        };

        /// Get the immediate domination path of a black
        /// \param block source block
        /// \param out destination path
        template<size_t C>
        void GetDominationPath(const BasicBlock* block, TrivialStackVector<const BasicBlock*, C>& out) {
            const BasicBlock* entryPoint = dominatorAnalysis->GetFunction().GetBasicBlocks().GetEntryPoint();
            
            while (block) {
                out.Add(block);

                // Entry point idom is itself
                if (block == entryPoint) {
                    return;
                }

                // Append by idom
                block = dominatorAnalysis->GetImmediateDominator(block);
            }
        }

        /// Intersect two domination paths
        /// \return common ancestor
        const IL::BasicBlock* IntersectDominationPath(const std::span<const BasicBlock*>& first, const std::span<const BasicBlock*>& second) {
            for (const BasicBlock* target : first) {
                // This could be faster, however, it's also not terribly bad on moderately complex programs
                for (const BasicBlock* test : second) {
                    if (test == target) {
                        return test;
                    }
                }
            }

            // None found
            return nullptr;
        }

        /// Intersect all intersection blocks of a phi instruction
        /// \param loop optional, loop information
        /// \param instr source instruction
        /// \param sharedAncestors all shared, unique, ancestors
        template<size_t C>
        void IntersectAllDominationPaths(const Loop* loop, std::span<const BasicBlock*> blocks, TrivialStackVector<const BasicBlock*, C>& sharedAncestors) {
            TrivialStackVector<const BasicBlock*, 128> dominationBlocks;
            TrivialStackVector<BlockRange, 32>   dominationRanges;

            // Populate all paths
            for (const BasicBlock* block : blocks) {
                size_t begin = dominationBlocks.Size();

                // If the branch is a back edge, ignore it
                // This is not analyzed by this propagator
                if (loop && loop->IsBackEdge(block)) {
                    continue;
                }

                // Populate domination for this branch
                GetDominationPath(block, dominationBlocks);
                dominationRanges.Add(BlockRange{ begin, dominationBlocks.Size() });
            }

            // Intersect all paths
            for (size_t i = 0; i < dominationRanges.Size(); i++) {
                for (size_t j = i + 1; j < dominationRanges.Size(); j++) {
                    const BlockRange& firstRange  = dominationRanges[i];
                    const BlockRange& secondRange = dominationRanges[j];

                    // Find the shared ancestor, must exist
                    const BasicBlock* ancestor = IntersectDominationPath(
                        std::span(dominationBlocks.Data() + firstRange.begin, firstRange.end - firstRange.begin),
                        std::span(dominationBlocks.Data() + secondRange.begin, secondRange.end - secondRange.begin)
                    );

                    // Ignore already visited
                    if (ancestor->HasFlag(BasicBlockFlag::Visited)) {
                        continue;
                    }

                    sharedAncestors.Add(ancestor);
                    ancestor->AddNonSemanticFlag(BasicBlockFlag::Visited);
                }
            }

            // Cleanup
            for (const BasicBlock *ancestor : sharedAncestors) {
                ancestor->RemoveNonSemanticFlag(BasicBlockFlag::Visited);
            }
        }

    private:
        struct MemoryTreeNode {
            /// All memory patterns
            std::vector<std::pair<ConstantPropagatorMemory::MemoryAddressNode, MemoryTreeNode*>> children;
        };

        struct DivergenceState {
            /// Divergence stored for this value
            WorkGroupDivergence divergence;

            /// Memory tree associated, may not be used
            MemoryTreeNode memory;
        };
        
        struct StoredDivergence {
            /// The memory pattern
            const MemoryTreeNode* memory = nullptr;

            /// Unique divergence stored at said pattern
            WorkGroupDivergence divergence;
        };

        struct ReachingCandidate {
            /// Stored divergence data
            StoredDivergence stored;

            /// Originating block
            const BasicBlock* block{nullptr};
        };

        struct MemoryTreeTraversal {
            /// Final node
            MemoryTreeNode* node{nullptr};

            /// Divergence of the whole traversal
            WorkGroupDivergence divergence = WorkGroupDivergence::Unknown;
        };

        /// Find a memory tree node from an address
        /// \param address address, will be walked back from
        /// \param instantiateMissingNodes if false, breaks traversal on missing nodes 
        /// \return traversal data
        MemoryTreeTraversal GetMemoryTreeNode(ID address, bool instantiateMissingNodes) {
            ID base;
            
            // Get the access chain
            ConstantPropagatorMemory::IDStack chain;
            base = constantPropagator.GetMemory()->PopulateAccessChain(address, chain);

            // Check the chain
            if (base == InvalidID) {
                return {};
            }

            // Get state
            DivergenceState& state = divergenceValues[base];

            // Traversal state
            MemoryTreeTraversal out;
            out.node = &state.memory;
            out.divergence = WorkGroupDivergence::Uniform;

            // Check all chains
            for (uint32_t i = 0; i < chain.Size(); i++) {
                MemoryTreeNode* next = nullptr;

                // Get address node from constant propagator
                const ConstantPropagatorMemory::MemoryAddressNode& addressNode = constantPropagator.GetMemory()->GetMemoryAddressNode(chain[i]);

                // Try to find matching memory child
                for (auto&& kv : out.node->children) {
                    if (kv.first == addressNode) {
                        next = kv.second;
                        break;
                    }
                }

                // None found?
                if (!next) {
                    if (!instantiateMissingNodes) {
                        break;
                    }

                    // Create memory child
                    next = new MemoryTreeNode;
                    out.node->children.emplace_back(addressNode, next);
                }

                out.node = next;
            }

            // Chain divergence checks is independent of actual chains
            // If a single node is divergent, the entire thing is, regardless if it's mapped or not
            for (uint32_t i = 0; i < chain.Size(); i++) {
                const ConstantPropagatorMemory::MemoryAddressNode& addressNode = constantPropagator.GetMemory()->GetMemoryAddressNode(chain[i]);
                if (addressNode.type == ConstantPropagatorMemory::MemoryAddressType::Varying && IsDivergent(addressNode.varying)) {
                    out.divergence = WorkGroupDivergence::Divergent;
                }
            }

            // OK
            return out;
        }

        /// Check if a value is divergent
        bool IsDivergent(ID id) {
            return divergenceValues.at(id).divergence == WorkGroupDivergence::Divergent;
        }

        /// Mark a value as divergent
        void MarkAsDivergent(ID id) {
            divergenceValues.at(id).divergence = WorkGroupDivergence::Divergent;
        }

        /// Mark a value as uniform
        void MarkAsUniform(ID id) {
            divergenceValues.at(id).divergence = WorkGroupDivergence::Uniform;
        }

        /// Set the divergence of a value
        void SetDivergence(ID id, WorkGroupDivergence divergence) {
            divergenceValues.at(id).divergence = divergence;
        }
        
        /// Find the reaching, i.e., dominating, store definition with a matching memory tree
        /// \param block block to search
        /// \param instr optional, top most instruction, iteration stops when met
        /// \param stores all stores
        /// \return search result
        template<size_t C>
        void FindReachingStoreDefinitions(const BasicBlock* block, const Instruction* instr, const MemoryTreeNode* memory, TrivialStackVector<ReachingCandidate, C>& stores) {
            FindReachingStoreDefinitionsChecked(block, instr, memory, stores);

            // Cleanup
            for (const BasicBlock* visited : function.GetBasicBlocks()) {
                visited->RemoveNonSemanticFlag(BasicBlockFlag::Visited);
            }
        }

        /// Find the reaching, i.e., dominating, store definition with a matching memory tree
        template<size_t C>
        void FindReachingStoreDefinitionsChecked(const BasicBlock* block, const Instruction* instr, const MemoryTreeNode* memory, TrivialStackVector<ReachingCandidate, C>& stores) {
            if (block->HasFlag(BasicBlockFlag::Visited)) {
                return;
            }

            // Search new path
            FindReachingStoreDefinitionsInner(block, instr, memory, stores);
            block->AddNonSemanticFlag(BasicBlockFlag::Visited);
        }

        /// Find the reaching, i.e., dominating, store definition with a matching memory tree
        template<size_t C>
        void FindReachingStoreDefinitionsInner(const BasicBlock* block, const Instruction* instr, const MemoryTreeNode* memory, TrivialStackVector<ReachingCandidate, C>& stores) {
            ReachingCandidate candidate{};
            
            // Search forward in the current block
            for (const Instruction* blockInstr : *block) {
                if (blockInstr == instr) {
                    break;
                }

                // Only interested in stores
                if (!blockInstr->Is<StoreInstruction>()) {
                    continue;
                }

                // Matching memory tree?
                auto it = ssaDivergenceLookup.find(blockInstr);
                if (it == ssaDivergenceLookup.end() || it->second.memory != memory) {
                    continue;
                }

                // Assign, do not terminate as the memory pattern by assigned again
                candidate = ReachingCandidate {
                    .stored = it->second,
                    .block = block
                };
            }

            // Found?
            if (candidate.block) {
                stores.Add(candidate);
                return;
            }

            const Loop* loopDefinition{nullptr};

            if (auto loopIt = loopLookup.find(block); loopIt != loopLookup.end()) {
                loopDefinition = loopIt->second;
            }

            // None found, check predecessors
            const DominatorAnalysis::BlockView& predecessors = dominatorAnalysis->GetPredecessors(block);
            if (predecessors.empty()) {
                return;
            }

            // If a single predecessor, search directly
            if (predecessors.size() == 1) {
                // Ignore back edges
                if (loopDefinition && loopDefinition->IsBackEdge(predecessors[0])) {
                    return;
                }

                FindReachingStoreDefinitionsChecked(predecessors[0], nullptr, memory, stores);
            }

            // Search all predecessors for candidates
            for (const BasicBlock* predecessor : predecessors) {
                // Ignore back edges
                if (loopDefinition && loopDefinition->IsBackEdge(predecessor)) {
                    continue;
                }

                // Ignore blocks branching to itself
                if (predecessor == block) {
                    continue;
                }

                // If the edge is not executable, we can ignore any contribution
                if (!propagationEngine->IsEdgeExecutable(predecessor, block)) {
                    continue;
                }

                // Find from all predecessors
                FindReachingStoreDefinitionsChecked(predecessor, nullptr, memory, stores);
            }
        }

    private:
        /// Outer constant propagator
        const ConstantPropagator& constantPropagator;
        
        /// Outer program
        Program& program;

        /// Source function
        Function& function;

        /// Installed engine
        Backend::IL::PropagationEngine* propagationEngine{nullptr};

        /// All propagated divergences (result wise lookup)
        std::vector<DivergenceState> divergenceValues;

        /// Constant analysis
        ComRef<ConstantPropagator> ConstantPropagator;

        /// Domination tree
        ComRef<DominatorAnalysis> dominatorAnalysis;

        /// Loop tree
        ComRef<LoopAnalysis> loopAnalysis;

    private:
        /// Divergence lookup for SSA instructions
        std::unordered_map<const Instruction*, StoredDivergence> ssaDivergenceLookup;

    private:
        /// Loop lookup
        std::unordered_map<const BasicBlock*, const Loop*> loopLookup;
    };
}
