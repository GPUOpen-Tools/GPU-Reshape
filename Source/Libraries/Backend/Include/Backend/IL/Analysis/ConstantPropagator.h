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
#include <Backend/IL/InstructionCommon.h>
#include <Backend/IL/InstructionAddressCommon.h>
#include <Backend/IL/Constant/ConstantFoldingCommon.h>
#include <Backend/IL/Constant/ConstantFolding.h>
#include <Backend/IL/Utils/PropagationEngine.h>
#include <Backend/IL/Analysis/UserAnalysis.h>
#include <Backend/IL/Analysis/CFG/DominatorAnalysis.h>
#include <Backend/IL/Analysis/CFG/LoopAnalysis.h>
#include <Backend/IL/Analysis/ConstantPropagatorMemory.h>

namespace IL {
    class ConstantPropagator {
    public:
        using Memory = ConstantPropagatorMemory;
        
        COMPONENT(ConstantPropagator);
        
        /// Constructor
        /// \param program program to inject constants to
        /// \param function function to compute constant analysis for
        /// \param propagationEngine shared propagation engine
        ConstantPropagator(Program& program, Function& function, Backend::IL::PropagationEngine& propagationEngine) :
            program(program), function(function),
            propagationEngine(propagationEngine) {
        }

        /// Compute constant propagation of a function
        bool Install() {
            // Compute instruction user analysis to ssa-edges
            if (users = program.GetAnalysisMap().FindPassOrCompute<UserAnalysis>(program); !users) {
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
            
            // Setup loop headers
            for (const Loop& loop : loopAnalysis->GetView()) {
                BlockInfo& info = blockLookup[loop.header];
                info.loop = &loop;
            }
            
            // OK
            return true;
        }

        /// Set the memory to be used, may be shared
        void SetMemory(Memory* externalMemory) {
            memory = externalMemory;
        }

        /// Set the memory to be used, may be shared
        Memory* GetMemory() const {
            return memory;
        }

        /// Get the local ssa memory
        const Memory::LocalSSAMemory& GetLocalSSAMemory() const {
            return ssaMemory;
        }

        /// Propagate an instruction and its side effects
        /// \param block source block
        /// \param instr instruction to propagate
        /// \param branchBlock output basic block
        /// \return final result
        Backend::IL::PropagationResult PropagateInstruction(const BasicBlock* block, const Instruction* instr, const BasicBlock** branchBlock) {
            switch (instr->opCode) {
                default: {
                    // Result-less instructions are defaulted to varying
                    if (instr->result == InvalidID) {
                        return Backend::IL::PropagationResult::Varying;
                    }

                    return PropagateResultInstruction(block, instr, branchBlock);
                }
                case OpCode::AddressChain: {
                    return PropagateAddressChainInstruction(block, instr->As<AddressChainInstruction>(), branchBlock);
                }
                case OpCode::Load: {
                    return PropagateLoadInstruction(block, instr->As<LoadInstruction>(), branchBlock);
                }
                case OpCode::Store: {
                    return PropagateStoreInstruction(block, instr->As<StoreInstruction>(), branchBlock);
                }
                case OpCode::Phi: {
                    return PropagatePhiInstruction(block, instr->As<PhiInstruction>(), branchBlock);
                }
                case OpCode::Branch: {
                    return PropagateBranchInstruction(block, instr->As<BranchInstruction>(), branchBlock);
                }
                case OpCode::BranchConditional: {
                    return PropagateBranchConditionalInstruction(block, instr->As<BranchConditionalInstruction>(), branchBlock);
                }
                case OpCode::Switch: {
                    return PropagateSwitchInstruction(block, instr->As<SwitchInstruction>(), branchBlock);
                }
            }
        }

        /// Propagate all side effects of a loop
        /// \param loop given loop definition
        void PropagateLoopEffects(const Loop* loop) {
            BlockInfo& blockInfo = blockLookup[loop->header];

            // Propgate all body blocks (includes edges)
            for (BasicBlock* block : loop->blocks) {
                for (const Instruction* instr : *block) {
                    if (!instr->Is<StoreInstruction>()) {
                        continue;
                    }

                    // Store may not have been resolved
                    auto it = ssaMemory.lookup.find(instr);
                    if (it == ssaMemory.lookup.end()) {
                        continue;
                    }

                    // Store resolved memory
                    blockInfo.memoryLookup[it->second.memory] = it->second;
                }
            }
        }

        /// Propagate state from a remote propagator
        /// \param remote propagator to fetch state from
        /// \param block source block, state from all predecessors are propagated
        void PropagateGlobalState(const ConstantPropagator& remote, const BasicBlock* block) {
            ComRef<DominatorAnalysis> remoteDominatorAnalysis;

            // Get the dominator tree of the remote function
            if (remoteDominatorAnalysis = remote.function.GetAnalysisMap().FindPassOrCompute<DominatorAnalysis>( remote.function); !remoteDominatorAnalysis) {
                return;
            }

            // Propagate all constants to local
            PropagateGlobalStateInner(remote, remoteDominatorAnalysis, block);
            
            // Cleanup
            for (BasicBlock* block : remote.function.GetBasicBlocks()) {
                block->RemoveFlag(BasicBlockFlag::Visited);
            }
        }

        /// Clear an instruction and intermediate data
        /// \param instr instruction to clear
        void ClearInstruction(const Instruction* instr) {
            ssaMemory.lookup.erase(instr);
        }

        /// Simulate a static store operation
        /// \param target destination memory to be written
        /// \param source source memory to transfer
        void StoreStatic(ID target, ID source) {
            memory->propagationValues[target] = memory->propagationValues[source];
        }

        /// Mark an identifier as varying
        /// \param id given identifier
        /// \return propagation result
        Backend::IL::PropagationResult MarkAsVarying(ID id) {
            Memory::PropagatedValue& value = memory->propagationValues[id];
            return value.lattice = Backend::IL::PropagationResult::Varying;
        }

        /// Mark an identifier as ignored
        /// \param id given identifier
        /// \return propagation result
        Backend::IL::PropagationResult MarkAsIgnored(ID id) {
            Memory::PropagatedValue& value = memory->propagationValues[id];
            return value.lattice = Backend::IL::PropagationResult::Ignore;
        }

        /// Mark an identifier as mapped
        /// \param id given identifier
        /// \param constant constant to be mapped
        /// \return propagation result
        Backend::IL::PropagationResult MarkAsMapped(ID id, const Constant* constant) {
            ASSERT(constant != nullptr, "Invalid mapping");

            Memory::PropagatedValue& value = memory->propagationValues[id];
            value.constant = constant;
            return value.lattice = Backend::IL::PropagationResult::Mapped;
        }

        /// Mark an identifier as overdefined
        /// \param id given identifier
        /// \return propagation result
        Backend::IL::PropagationResult MarkAsOverdefined(ID id) {
            Memory::PropagatedValue& value = memory->propagationValues[id];
            value.constant = nullptr;
            return value.lattice = Backend::IL::PropagationResult::Overdefined;
        }

        /// Check if an identifier is a constant
        bool IsConstant(ID id) const {
            return memory->propagationValues.at(id).lattice == Backend::IL::PropagationResult::Mapped;
        }

        /// Check if an identifier is a partial constant
        /// Composite types may be partially mapped, such as arrays ([1, 2, -, 4], but 3 not mapped)
        bool IsPartialConstant(ID id) const {
            const Memory::PropagatedValue& value = memory->propagationValues.at(id);
            return value.constant && value.lattice == Backend::IL::PropagationResult::Varying;
        }

        /// Check if an identifier is presumed varying (i.e., not constant)
        bool IsVarying(ID id) const {
            // Note that we are checking for a lack of mapping, not the propagation result
            // It may not have been propagated at all
            return memory->propagationValues.at(id).lattice != Backend::IL::PropagationResult::Mapped;
        }

        /// Check if an identifier is overdefined (i.e., has multiple compile time values)
        bool IsOverdefined(ID id) const {
            return memory->propagationValues.at(id).lattice == Backend::IL::PropagationResult::Overdefined;
        }

    public:
        /// Load a constant from an address
        /// \param block block requesting the load
        /// \param instr instruction requesting the load, affects general memory visibility
        /// \param address the specific address requested
        /// \return nullptr, if not found or unreachable
        const Constant* LoadAddress(const BasicBlock* block, const Instruction* instr, ID address) {
            // Get the access chain
            Memory::IDStack chain;
            ID base = memory->PopulateAccessChain(address, chain);

            // Check the chain
            if (base == InvalidID) {
                return nullptr;
            }

            // If a value is constant at this point it's either non-composite, or
            // is a global value which may be composite. Global composites are unwrapped
            // via the memory tree for later composition.
            Memory::PropagatedValue& value = memory->propagationValues[base];
            if (value.constant) {
                // Try to traverse
                const Constant* constant = memory->TraverseImmediateConstant(value.constant, chain);
                if (!constant) {
                    return nullptr;
                }

                // OK
                return constant;
            }

            // Get the range associated with the value
            // We are not checking for the lattice here, as memory ranges can differ
            Memory::PropagatedMemoryRange* range = memory->GetMemoryRange(value);

            // Try to associate memory
            Memory::PropagatedMemoryTraversal traversal = memory->FindPropagatedMemory(chain, range);
            if (!traversal.match) {
                // If addressing into a constant of unknown origins, treat it as mapped but unexposed
                if (traversal.partialMatch && traversal.partialMatch->memory->value) {
                    return program.GetConstants().AddSymbolicConstant(
                        program.GetTypeMap().GetType(address)->As<Backend::IL::PointerType>()->pointee,
                        UnexposedConstant{}
                    );
                }
            
                return nullptr;
            }

            // Find the reaching store
            ReachingStoreResult version = FindReachingStoreDefinition(block, instr, traversal.match->memory);
            if (!version.version) {
                return nullptr;
            }

            // OK!
            return traversal.match->memory->value;
        }

    private:
        /// Propagation case handler
        Backend::IL::PropagationResult PropagateAddressChainInstruction(const BasicBlock* block, const AddressChainInstruction* instr, const BasicBlock** branchBlock) {
            // Address chains "keep" the value around,
            // it's incredibly useful for features to be aware of what the chain saw during propagation,
            // the address or contents may change after this instruction.
            return PropagateAddressValueInstruction(block, instr, instr->composite, branchBlock);
        }
        
        /// Propagation case handler
        Backend::IL::PropagationResult PropagateLoadInstruction(const BasicBlock* block, const LoadInstruction* instr, const BasicBlock** branchBlock) {
            return PropagateAddressValueInstruction(block, instr, instr->address, branchBlock);
        }
        
        /// Propagation case handler
        Backend::IL::PropagationResult PropagateAddressValueInstruction(const BasicBlock* block, const Instruction* instr, ID address, const BasicBlock** branchBlock) {
            // Get the pointer type
            const auto* type = program.GetTypeMap().GetType(address)->As<Backend::IL::PointerType>();

            // If an external address space, don't try to assume the value
            if (type->addressSpace != Backend::IL::AddressSpace::Function) {
                return MarkAsVarying(instr->result);
            }

            // Try to load from address
            const Constant* constant = LoadAddress(block, instr, address);
            if (!constant) {
                return MarkAsVarying(instr->result);
            }

            // OK!
            return MarkAsMapped(instr->result, constant);
        }

        /// Propagation case handler
        Backend::IL::PropagationResult PropagateStoreInstruction(const BasicBlock* block, const StoreInstruction* instr, const BasicBlock** branchBlock) {
            // Get the pointer type
            const auto* type = program.GetTypeMap().GetType(instr->address)->As<Backend::IL::PointerType>();

            // If an external address space, don't try to assume the value
            if (type->addressSpace != Backend::IL::AddressSpace::Function) {
                return Backend::IL::PropagationResult::Varying;
            }

            // Value must be constant
            const Memory::PropagatedValue& storeValue = memory->propagationValues[instr->value];
            if (storeValue.lattice != Backend::IL::PropagationResult::Mapped) {
                return Backend::IL::PropagationResult::Ignore;
            }

            // Get the access chain
            Memory::IDStack chain;
            ID base = memory->PopulateAccessChain(instr->address, chain);

            // Check the chain
            if (base == InvalidID) {
                return Backend::IL::PropagationResult::Varying;
            }

            // Get the range associated with the value
            // We are not checking for the lattice here, as memory ranges can differ
            Memory::PropagatedMemoryRange* range = memory->GetMemoryRange(memory->propagationValues[base]);

            // Write memory instance
            Memory::MemoryAccessTreeNode* propagatedMemory = memory->FindOrCreatePropagatedMemory(chain, range);
            propagatedMemory->memory->lattice = Backend::IL::PropagationResult::Mapped;
            propagatedMemory->memory->value = storeValue.constant;

            // Instantiate a new memory tree in place
            memory->CreateMemoryTree(propagatedMemory, storeValue.constant);

            // Set SSA lookup
            ssaMemory.lookup[instr] = Memory::PropagatedMemorySSAVersion {
                .memory = propagatedMemory->memory,
                .value = storeValue.constant
            };

            // Inform the propagator that this has been mapped, without assigning a value to it
            return propagatedMemory->memory->lattice;
        }

        /// Propagation case handler
        Backend::IL::PropagationResult PropagatePhiInstruction(const BasicBlock* block, const PhiInstruction* instr, const BasicBlock** branchBlock) {
            const Constant* phiConstant = nullptr;

            for (uint32_t i = 0; i < instr->values.count; i++) {
                PhiValue phiValue = instr->values[i];

                if (!propagationEngine.IsEdgeExecutable(program.GetIdentifierMap().GetBasicBlock(phiValue.branch), block)) {
                    continue;
                }

                // todo: why return?
                const Memory::PropagatedValue& value = memory->propagationValues[phiValue.value];
                switch (value.lattice) {
                    default:
                        break;
                    case Backend::IL::PropagationResult::Varying:
                    case Backend::IL::PropagationResult::Overdefined:
                        return MarkAsVarying(instr->result);
                    case Backend::IL::PropagationResult::None:
                    case Backend::IL::PropagationResult::Ignore:
                        continue;
                }

                // If first, assume the constant, otherwise must match (implies multiple incoming edges, cannot be reduced)
                if (!phiConstant || phiConstant == value.constant) {
                    phiConstant = value.constant;
                    continue;
                }

                return MarkAsVarying(instr->result);
            }

            if (!phiConstant) {
                return Backend::IL::PropagationResult::Ignore;
            }

            return MarkAsMapped(instr->result, phiConstant);
        }

        /// Propagation case handler
        Backend::IL::PropagationResult PropagateBranchInstruction(const BasicBlock* block, const BranchInstruction* instr, const BasicBlock** branchBlock) {
            *branchBlock = program.GetIdentifierMap().GetBasicBlock(instr->branch);
            return Backend::IL::PropagationResult::Mapped;
        }

        /// Propagation case handler
        Backend::IL::PropagationResult PropagateBranchConditionalInstruction(const BasicBlock* block, const BranchConditionalInstruction* instr, const BasicBlock** branchBlock) {
            const Memory::PropagatedValue& value = memory->propagationValues[instr->cond];
            switch (value.lattice) {
                default:
                    break;
                case Backend::IL::PropagationResult::Varying:
                case Backend::IL::PropagationResult::Overdefined:
                case Backend::IL::PropagationResult::Ignore:
                case Backend::IL::PropagationResult::None:
                    return Backend::IL::PropagationResult::Varying;
            }

            // If unexposed, consider it varying, which will visit both branches
            if (value.constant->Is<UnexposedConstant>()) {
                return Backend::IL::PropagationResult::Varying;
            }

            // Determine branch
            ID branch;
            if (value.constant->As<BoolConstant>()->value) {
                branch = instr->pass;
            } else {
                branch = instr->fail;
            }

            *branchBlock = program.GetIdentifierMap().GetBasicBlock(branch);
            return Backend::IL::PropagationResult::Mapped;
        }

        /// Propagation case handler
        Backend::IL::PropagationResult PropagateSwitchInstruction(const BasicBlock* block, const SwitchInstruction* instr, const BasicBlock** branchBlock) {
            const Memory::PropagatedValue& value = memory->propagationValues[instr->value];
            switch (value.lattice) {
                default:
                    break;
                case Backend::IL::PropagationResult::Varying:
                case Backend::IL::PropagationResult::Overdefined:
                case Backend::IL::PropagationResult::Ignore:
                case Backend::IL::PropagationResult::None:
                    return Backend::IL::PropagationResult::Varying;
            }

            for (uint32_t i = 0; i < instr->cases.count; i++) {
                SwitchCase _case = instr->cases[i];

                if (_case.literal == value.constant->id) {
                    *branchBlock = program.GetIdentifierMap().GetBasicBlock(_case.branch);
                    return Backend::IL::PropagationResult::Mapped;
                }
            }

            // Try to get default block
            *branchBlock = program.GetIdentifierMap().GetBasicBlock(instr->_default);
            if (!*branchBlock) {
                ASSERT(false, "Switch propagation without a viable edge");
                return Backend::IL::PropagationResult::Varying;
            }

            // OK
            return Backend::IL::PropagationResult::Mapped;
        }

        /// Propagation case handler
        Backend::IL::PropagationResult PropagateResultInstruction(const BasicBlock* block, const Instruction* instr, const BasicBlock** branchBlock) {
            // Check if the instruction can be folded at all
            if (!Backend::IL::CanFoldWithImmediates(instr)) {
                return MarkAsVarying(instr->result);
            }

            // Operand info
            bool anyUnmapped    = false;
            bool anyVarying     = false;
            bool anyOverdefined = false;
            bool anyUnexposed   = false;

            // Gather all operands
            Backend::IL::VisitOperands(instr, [&](ID id) {
                const Memory::PropagatedValue& value = memory->propagationValues[id];
                anyVarying     |= value.lattice == Backend::IL::PropagationResult::Varying;
                anyOverdefined |= value.lattice == Backend::IL::PropagationResult::Overdefined;
                anyUnmapped    |= value.lattice == Backend::IL::PropagationResult::None ||
                                  value.lattice == Backend::IL::PropagationResult::Ignore ||
                                  (value.constant && value.constant->Is<UndefConstant>());
                anyUnexposed   |= value.constant && value.constant->Is<UnexposedConstant>();
            });

            // If any operands are varying, this instruction will be too
            // Special case for overdefined values, we don't inherit those
            if (anyVarying || anyOverdefined) {
                return MarkAsVarying(instr->result);
            }

            // If any operands are unmapped, skip it
            if (anyUnmapped) {
                return MarkAsIgnored(instr->result);
            }

            // Special exception, if any of the operands are unexposed, treat it as mapped
            if (anyUnexposed) {
                return MarkAsMapped(instr->result, program.GetConstants().AddSymbolicConstant(
                    program.GetTypeMap().GetType(instr->result),
                    UnexposedConstant{}
                ));
            }

            // Try to fold the instruction
            const Constant* constant = Backend::IL::FoldConstantInstruction(program, instr, [&](IL::ID id) {
                const Memory::PropagatedValue& value = memory->propagationValues[id];
                ASSERT(value.lattice == Backend::IL::PropagationResult::Mapped, "Mapping invalid constant");
                return value.constant;
            });

            // If the folding failed at this point, it'll never fold
            if (!constant) {
                return MarkAsVarying(instr->result);
            }

            // Successfully folded!
            return MarkAsMapped(instr->result, constant);
        }

    private:
        /// Does the lattice have any data?
        bool IsStatefulLattice(Backend::IL::PropagationResult lattice) {
            return static_cast<uint32_t>(lattice) > static_cast<uint32_t>(Backend::IL::PropagationResult::Ignore);
        }

        /// Join two memory lattices
        Backend::IL::PropagationResult JoinMemoryLattice(Backend::IL::PropagationResult before, Backend::IL::PropagationResult after) {
            // If there's no state, just assign it
            if (!IsStatefulLattice(before)) {
                return after;
            }

            // If there's two states, it's overdefined
            if (IsStatefulLattice(after)) {
                return Backend::IL::PropagationResult::Overdefined;
            }

            // No state in either, just presume ok
            return after;
        }

    private:
        /// TODO: Reduce allocations

        struct BlockInfo {
            /// Optional, outer loop definition
            const Loop* loop{nullptr};
            
            /// All merged side effects of an iteration
            std::unordered_map<const Memory::PropagatedMemory*, Memory::PropagatedMemorySSAVersion> memoryLookup;
        };

        struct ReachingStoreResult {
            /// Result of the store search
            /// May be overdefined on ambiguous searches
            Backend::IL::PropagationResult result{Backend::IL::PropagationResult::None};

            /// Found version
            Memory::PropagatedMemorySSAVersion* version{nullptr};
        };
        
        struct ReachingStoreCache {
            /// All memoized blocks
            std::map<const BasicBlock*, ReachingStoreResult> blockMemoization;
        };

        /// Find the reaching, i.e., dominating, store definition with a matching memory tree
        /// \param block block to search
        /// \param instr optional, top most instruction, iteration stops when met
        /// \param memory memory leaf to search for, matches by address
        /// \return searech result
        ReachingStoreResult FindReachingStoreDefinition(const BasicBlock* block, const Instruction* instr, const Memory::PropagatedMemory* memory) {
            ReachingStoreCache cache;
            return FindReachingStoreDefinition(block, instr, memory, cache);
        }

        /// Find the reaching, i.e., dominating, store definition with a matching memory tree
        ReachingStoreResult FindReachingStoreDefinition(const BasicBlock* block, const Instruction* instr, const Memory::PropagatedMemory* memory, ReachingStoreCache& cache) {
            // Check memoization
            if (auto it = cache.blockMemoization.find(block); it != cache.blockMemoization.end()) {
                return it->second;
            }

            // Search new path
            ReachingStoreResult result = FindReachingStoreDefinitionInner(block, instr, memory, cache);

            // OK
            cache.blockMemoization[block] = result;
            return result;
        }

        /// Find the reaching, i.e., dominating, store definition with a matching memory tree
        ReachingStoreResult FindReachingStoreDefinitionInner(const BasicBlock* block, const Instruction* instr, const Memory::PropagatedMemory* memory, ReachingStoreCache& cache) {
            ReachingStoreResult result;

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
                auto it = ssaMemory.lookup.find(blockInstr);
                if (it == ssaMemory.lookup.end() || it->second.memory != memory) {
                    continue;
                }

                // Assign, do not terminate as the memory pattern by assigned again
                result.result = Backend::IL::PropagationResult::Mapped;
                result.version = &it->second;
                break;
            }

            // Found?
            if (result.version) {
                return result;
            }

            const Loop* loopDefinition{nullptr};

            // Before checking the predecessor trees, check if this is a look header,
            // and if the loop header has a collapsed set of memory ranges
            if (auto loopIt = blockLookup.find(block); loopIt != blockLookup.end()) {
                BlockInfo& info = loopIt->second;
                loopDefinition = info.loop;

                // Check if the memory pattern exists
                // Note that address checks on the loop memory ranges is fine, as it should be unique anyway
                if (auto memoryIt = info.memoryLookup.find(memory); memoryIt != info.memoryLookup.end()) {
                    result.result = Backend::IL::PropagationResult::Mapped;
                    result.version = &memoryIt->second;
                    return result;
                }
            }

            // None found, check predecessors
            const DominatorAnalysis::BlockView& predecessors = dominatorAnalysis->GetPredecessors(block);
            if (predecessors.empty()) {
                return {};
            }

            // If a single predecessor, search directly
            if (predecessors.size() == 1) {
                // Ignore back edges
                if (loopDefinition && loopDefinition->IsBackEdge(predecessors[0])) {
                    return {};
                }

                return FindReachingStoreDefinition(predecessors[0], nullptr, memory, cache);
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
                if (!propagationEngine.IsEdgeExecutable(predecessor, block)) {
                    continue;
                }

                ReachingStoreResult store = FindReachingStoreDefinition(predecessor, nullptr, memory, cache);
                if (store.result == Backend::IL::PropagationResult::Overdefined) {
                    return store;
                }

                // Nothing found at all?
                // Path itself was not of interest, just continue
                if (!store.version) {
                    continue;
                }

                // If there's already a candidate, and it didn't resolve to the same one we cannot safely proceed
                // Mark it as overdefined and let the caller handle it
                if (result.version && result.version != store.version) {
                    result.result = Backend::IL::PropagationResult::Overdefined;
                    result.version = nullptr;
                    return result;
                }

                // Mark candidate
                result.result = Backend::IL::PropagationResult::Mapped;
                result.version = store.version;
            }

            // OK
            return result;
        }

        /// Propagate all global state from a remote propagator
        void PropagateGlobalStateInner(const ConstantPropagator& remote, const ComRef<DominatorAnalysis>& remoteDominatorAnalysis, const BasicBlock* block) {
            if (block->HasFlag(BasicBlockFlag::Visited)) {
                return;
            }

            // Mark as visited
            block->AddNonSemanticFlag(BasicBlockFlag::Visited);

            // Remote memory to propagate
            const Memory::LocalSSAMemory& remoteLocal = remote.GetLocalSSAMemory();

            // Collapse all global state to the entry point block (reachable by everything)
            BlockInfo& entryBlockInfo = blockLookup[*function.GetBasicBlocks().begin()];
            
            // Search forward in the current block
            for (const Instruction* blockInstr : *block) {
                if (!blockInstr->Is<StoreInstruction>()) {
                    continue;
                }

                // Matching memory tree?
                auto it = remoteLocal.lookup.find(blockInstr);
                if (it == remoteLocal.lookup.end()) {
                    continue;
                }

                if (!entryBlockInfo.memoryLookup.contains(it->second.memory)) {
                    entryBlockInfo.memoryLookup[it->second.memory] = it->second;
                }
            }

            // Inherit all previously collapsed memory
            if (auto blockInfo = blockLookup.find(block); blockInfo != blockLookup.end()) {
                for (auto&& kv : blockInfo->second.memoryLookup) {
                    if (!entryBlockInfo.memoryLookup.contains(kv.first)) {
                        entryBlockInfo.memoryLookup[kv.first] = kv.second;
                    }
                }
            }

            // Search all predecessors for candidates
            for (const BasicBlock* predecessor : remoteDominatorAnalysis->GetPredecessors(block)) {
                PropagateGlobalStateInner(remote, remoteDominatorAnalysis, predecessor);
            }
        }

    private:
        /// Outer program
        Program& program;

        /// Source function
        Function& function;

        /// Domination tree
        ComRef<DominatorAnalysis> dominatorAnalysis;

        /// Loop tree
        ComRef<LoopAnalysis> loopAnalysis;

        /// All users
        ComRef<UserAnalysis> users;

    private:
        /// Underlying propagation engine
        Backend::IL::PropagationEngine& propagationEngine;

        /// Shared memory
        ConstantPropagatorMemory* memory{nullptr};

    private:
        /// Look lookup
        std::unordered_map<const BasicBlock*, BlockInfo> blockLookup;

        /// All local memory
        Memory::LocalSSAMemory ssaMemory;
    };
}
