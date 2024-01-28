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

namespace IL {
    class ConstantAnalysis {
    public:
        /// Constructor
        /// \param dominatorAnalysis block and instruction dominance
        /// \param loopAnalysis computed loop constructs
        ConstantAnalysis(Program& program, const DominatorAnalysis &dominatorAnalysis, const LoopAnalysis& loopAnalysis) :
            program(program),
            dominatorAnalysis(dominatorAnalysis),
            loopAnalysis(loopAnalysis),
            users(program),
            propagationEngine(program, dominatorAnalysis, loopAnalysis, users) {
            for (const Loop& loop : loopAnalysis.GetView()) {
                LoopInfo& info = loopLookup[loop.header];
                info.definition = &loop;
            }
        }

        ~ConstantAnalysis() {
            // Cleanup
            for (PropagatedValue& value : propagationValues) {
                if (value.memory) {
                    delete value.memory;
                }
            }
        }

        /// Compute constant propagation of a block list
        /// \param basicBlocks all basic blocks
        void Compute(const BasicBlockList& basicBlocks) {
            propagationValues.resize(program.GetIdentifierMap().GetMaxID());

            // Set program wide constants
            for (const Constant* constant : program.GetConstants()) {
                if (constant->IsSymbolic()) {
                    continue;
                }

                propagationValues[constant->id] = PropagatedValue {
                    .lattice = Backend::IL::PropagationResult::Mapped,
                    .constant = constant
                };
            }

            // Set global variable constants
            for (const Backend::IL::Variable* variable : program.GetVariableList()) {
                if (!variable->initializer) {
                    continue;
                }

                PropagatedValue value {
                    .lattice = Backend::IL::PropagationResult::Mapped,
                    .constant = nullptr
                };

                // Global variables require a fully visible memory tree
                switch (variable->initializer->type->kind) {
                    default:
                        value.constant = variable->initializer;
                        break;
                    case Backend::IL::TypeKind::Struct:
                    case Backend::IL::TypeKind::Array:
                        CreateMemoryTreeFromImmediate(&GetMemoryRange(value)->tree, variable->initializer);
                        break;
                }

                propagationValues[variable->id] = value;
            }

            // Compute all users
            users.Compute();

            // Compute propagation
            propagationEngine.Compute(basicBlocks, *this);

            // Finally, composite all memory ranges back into the typical constant layout
            CompositePropagatedMemoryRanges();
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
                    // Chains themselves, i.e. the memory addresses, are never known
                    return MarkAsVarying(instr->result);
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
            LoopInfo& loopInfo = loopLookup[loop->header];

            // Propgate all body blocks (includes edges)
            for (BasicBlock* block : loop->blocks) {
                for (const Instruction* instr : *block) {
                    if (!instr->Is<StoreInstruction>()) {
                        continue;
                    }

                    // Store may not have been resolved
                    auto it = ssaMemoryLookup.find(instr);
                    if (it == ssaMemoryLookup.end()) {
                        continue;
                    }

                    // Store resolved memory
                    loopInfo.memoryLookup[it->second.memory] = it->second;
                }
            }
        }

        /// Clear an instruction and intermediate data
        /// \param instr instruction to clear
        void ClearInstruction(const Instruction* instr) {
            ssaMemoryLookup.erase(instr);
        }

        /// Mark an identifier as varying
        /// \param id given identifier
        /// \return propagation result
        Backend::IL::PropagationResult MarkAsVarying(ID id) {
            PropagatedValue& value = propagationValues[id];
            return value.lattice = Backend::IL::PropagationResult::Varying;
        }

        /// Mark an identifier as mapped
        /// \param id given identifier
        /// \param constant constant to be mapped
        /// \return propagation result
        Backend::IL::PropagationResult MarkAsMapped(ID id, const Constant* constant) {
            ASSERT(constant != nullptr, "Invalid mapping");

            PropagatedValue& value = propagationValues[id];
            value.constant = constant;
            return value.lattice = Backend::IL::PropagationResult::Mapped;
        }

        /// Mark an identifier as overdefined
        /// \param id given identifier
        /// \return propagation result
        Backend::IL::PropagationResult MarkAsOverdefined(ID id) {
            PropagatedValue& value = propagationValues[id];
            value.constant = nullptr;
            return value.lattice = Backend::IL::PropagationResult::Overdefined;
        }

        /// Check if an identifier is a constant
        bool IsConstant(ID id) {
            return propagationValues.at(id).lattice == Backend::IL::PropagationResult::Mapped;
        }

        /// Check if an identifier is a partial constant
        /// Composite types may be partially mapped, such as arrays ([1, 2, -, 4], but 3 not mapped)
        bool IsPartialConstant(ID id) {
            const PropagatedValue& value = propagationValues.at(id);
            return value.constant && value.lattice == Backend::IL::PropagationResult::Varying;
        }

        /// Check if an identifier is presumed varying (i.e., not constant)
        bool IsVarying(ID id) {
            // Note that we are checking for a lack of mapping, not the propagation result
            // It may not have been propagated at all
            return propagationValues.at(id).lattice != Backend::IL::PropagationResult::Mapped;
        }

        /// Check if an identifier is overdefined (i.e., has multiple compile time values)
        bool IsOverdefined(ID id) {
            return propagationValues.at(id).lattice == Backend::IL::PropagationResult::Overdefined;
        }

    private:
        /// Propagation case handler
        Backend::IL::PropagationResult PropagateLoadInstruction(const BasicBlock* block, const LoadInstruction* instr, const BasicBlock** branchBlock) {
            // Get the pointer type
            const auto* type = program.GetTypeMap().GetType(instr->address)->As<Backend::IL::PointerType>();

            // If an external address space, don't try to assume the value
            if (type->addressSpace != Backend::IL::AddressSpace::Function) {
                return MarkAsVarying(instr->result);
            }

            // Get the access chain
            IDStack chain;
            ID base = PopulateAccessChain(instr->address, chain);

            // Check the chain
            if (base == InvalidID) {
                return Backend::IL::PropagationResult::Varying;
            }

            // If a value is constant at this point it's either non-composite, or
            // is a global value which may be composite. Global composites are unwrapped
            // via the memory tree for later composition.
            PropagatedValue& value = propagationValues[base];
            if (value.constant) {
                // Try to traverse
                const Constant* constant = TraverseImmediateConstant(value.constant, chain);
                if (!constant) {
                    return MarkAsVarying(instr->result);
                }

                // OK
                return MarkAsMapped(instr->result, constant);
            }

            // Get the range associated with the value
            // We are not checking for the lattice here, as memory ranges can differ
            PropagatedMemoryRange* range = GetMemoryRange(value);

            // Try to associate memory
            PropagatedMemory* memory = FindPropagatedMemory(chain, range);
            if (!memory) {
                return Backend::IL::PropagationResult::Ignore;
            }

            // Find the reaching store
            ReachingStoreResult version = FindReachingStoreDefinition(block, instr, memory);
            if (!version.version) {
                return Backend::IL::PropagationResult::Ignore;
            }

            // OK!
            return MarkAsMapped(instr->result, memory->value);
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
            const PropagatedValue& storeValue = propagationValues[instr->value];
            if (storeValue.lattice != Backend::IL::PropagationResult::Mapped) {
                return Backend::IL::PropagationResult::Ignore;
            }

            // Get the access chain
            IDStack chain;
            ID base = PopulateAccessChain(instr->address, chain);

            // Check the chain
            if (base == InvalidID) {
                return Backend::IL::PropagationResult::Varying;
            }

            // Get the range associated with the value
            // We are not checking for the lattice here, as memory ranges can differ
            PropagatedMemoryRange* range = GetMemoryRange(propagationValues[base]);

            // Write memory instance
            PropagatedMemory* memory = FindOrCreatePropagatedMemory(chain, range);
            memory->lattice = Backend::IL::PropagationResult::Mapped;
            memory->value = storeValue.constant;

            // Set SSA lookup
            ssaMemoryLookup[instr] = PropagatedMemorySSAVersion {
                .memory = memory,
                .value = storeValue.constant
            };

            // Inform the propagator that this has been mapped, without assigning a value to it
            return memory->lattice;
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
                const PropagatedValue& value = propagationValues[phiValue.value];
                switch (value.lattice) {
                    default:
                        break;
                    case Backend::IL::PropagationResult::Varying:
                    case Backend::IL::PropagationResult::Overdefined:
                        return MarkAsVarying(instr->result);
                    case Backend::IL::PropagationResult::None:
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
            const PropagatedValue& value = propagationValues[instr->cond];
            switch (value.lattice) {
                default:
                    break;
                case Backend::IL::PropagationResult::Varying:
                case Backend::IL::PropagationResult::Overdefined:
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
            const PropagatedValue& value = propagationValues[instr->value];
            switch (value.lattice) {
                default:
                    break;
                case Backend::IL::PropagationResult::Varying:
                case Backend::IL::PropagationResult::Overdefined:
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
                return Backend::IL::PropagationResult::Varying;
            }

            // Operand info
            bool anyUnmapped    = false;
            bool anyVarying     = false;
            bool anyOverdefined = false;
            bool anyUnexposed   = false;

            // Gather all operands
            Backend::IL::VisitOperands(instr, [&](ID id) {
                const PropagatedValue& value = propagationValues[id];
                anyVarying     |= value.lattice == Backend::IL::PropagationResult::Varying;
                anyOverdefined |= value.lattice == Backend::IL::PropagationResult::Overdefined;
                anyUnmapped    |= value.lattice == Backend::IL::PropagationResult::None || (value.constant && value.constant->Is<UndefConstant>());
                anyUnexposed   |= value.constant && value.constant->Is<UnexposedConstant>();
            });

            // If any operands are varying, this instruction will be too
            // Special case for overdefined values, we don't inherit those
            if (anyVarying || anyOverdefined) {
                return MarkAsVarying(instr->result);
            }

            // If any operands are unmapped, skip it
            if (anyUnmapped) {
                return Backend::IL::PropagationResult::Ignore;
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
                const PropagatedValue& value = propagationValues[id];
                ASSERT(value.lattice == Backend::IL::PropagationResult::Mapped, "Mapping invalid constant");
                return value.constant;
            });

            // If the folding failed at this point, it'll never fold
            if (!constant) {
                return Backend::IL::PropagationResult::Varying;
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
        enum class MemoryAddressType {
            None,
            Varying,
            Constant
        };

        struct MemoryAddressNode {
            bool operator==(const MemoryAddressNode& rhs) const {
                switch (type) {
                    case MemoryAddressType::None:
                        return type == rhs.type;
                    case MemoryAddressType::Varying:
                        return type == rhs.type && varying == rhs.varying;
                    case MemoryAddressType::Constant:
                        return type == rhs.type && constant == rhs.constant;
                }

                return false;
            }

            MemoryAddressType type{MemoryAddressType::None};

            union {
                IL::ID varying;
                const IL::Constant* constant;
            };
        };

        struct MemoryAddressChain {
            /// All nodes
            MemoryAddressNode* nodes{nullptr};

            /// Number of nodes
            uint32_t count{0};
        };

        struct PropagatedMemory {
            /// Memory lattice value
            Backend::IL::PropagationResult lattice{Backend::IL::PropagationResult::None};

            /// Reference used for the memory location
            MemoryAddressChain addressChain;

            /// The assigned constant to the reference address
            const Constant* value{nullptr};
        };

        struct PropagatedMemorySSAVersion {
            /// The memory target
            PropagatedMemory* memory{nullptr};
            
            /// The assigned constant to the reference address
            const Constant* value{nullptr};
        };

        struct MemoryAccessTreeNode {
            /// Memory associated with this node, may be null
            PropagatedMemory* memory{nullptr};

            /// All tree-wise children to this node
            std::vector<std::pair<MemoryAddressNode, MemoryAccessTreeNode*>> children;
        };

        struct PropagatedMemoryRange {
            /// All lattice values, linearly laid out
            std::vector<PropagatedMemory*> values;

            /// Tree layout
            MemoryAccessTreeNode tree;
        };

        struct PropagatedValue {
            /// Current lattice value
            Backend::IL::PropagationResult lattice{Backend::IL::PropagationResult::None};

            /// Optional, memory range on indirections
            /// Not that each reference may have a different lattice value
            PropagatedMemoryRange* memory{nullptr};

            /// Optional, assigned constant on mapped lattices
            const Constant* constant{nullptr};
        };

        /// Helper, identifier stack for searches
        using IDStack = TrivialStackVector<ID, 32>;

        /// Helper, node stack for searches
        using MemoryAddressNodeStack = TrivialStackVector<MemoryAddressNode, 32>;

        /// Get the memory range for a value
        PropagatedMemoryRange* GetMemoryRange(PropagatedValue& value) {
            if (!value.memory) {
                value.memory = new PropagatedMemoryRange();
            }

            return value.memory;
        }

        /// Check if an address chain base offset is non-zero
        bool IsBaseOffsetNonConstantZero(ID id) {
            const Constant* constant = program.GetConstants().GetConstant(id);
            if (!constant) {
                return true;
            }

            // Must be constant
            auto _int = constant->As<IntConstant>();
            if (!_int) {
                return false;
            }

            return _int->value != 0;
        }

        /// Get the access chain from an identifier
        /// \param id id to populate from
        /// \param chain output chain
        /// \return base composite or indirection
        ID PopulateAccessChain(ID id, IDStack& chain) {
            // All address chains must start with the base offset, which is typically zero.
            // However, some languages allow for base offsets before dereferencing the composite address.
            // This is not supported by constant analysis.
            bool hasBaseCompositeOffset = false;

            // Walk reverse address back (index -> ... -> allocation)
            Backend::IL::VisitGlobalAddressChainReverse(program, id, [&](ID id, bool isCompositeBase) {
                if (isCompositeBase) {
                    hasBaseCompositeOffset |= IsBaseOffsetNonConstantZero(id);
                    return;
                }

                chain.Add(id);
            });

            // Nothing?
            if (!chain.Size() || hasBaseCompositeOffset) {
                return InvalidID;
            }

            // Don't report the base address
            ID base = chain.PopBack();

            // Reverse in-place
            std::ranges::reverse(chain);

            return base;
        }

        /// Find the propagated memory
        PropagatedMemory* FindPropagatedMemory(const IDStack& chain, PropagatedMemoryRange* range) {
            MemoryAddressNodeStack ignore;
            return FindPropagatedMemory(chain, range, ignore);
        }

        /// Find the propagated memory
        PropagatedMemory* FindPropagatedMemory(const IDStack& chain, PropagatedMemoryRange* range, MemoryAddressNodeStack& workingNodes) {
            workingNodes.Resize(chain.Size());

            // Fill working nodes
            for (uint32_t i = 0; i < chain.Size(); i++) {
                workingNodes[i] = GetMemoryAddressNode(chain[i]);
            }

            // Root tree
            MemoryAccessTreeNode* treeNode = &range->tree;

            for (uint32_t i = 0; i < workingNodes.Size(); i++) {
                MemoryAccessTreeNode* nextTree = nullptr;

                // Try to find node
                for (auto && tag : treeNode->children) {
                    if (tag.first == workingNodes[i]) {
                        nextTree = tag.second;
                        break;
                    }
                }

                if (!nextTree) {
                    return nullptr;
                }

                treeNode = nextTree;
            }

            // OK
            return treeNode->memory;
        }

        /// Find or create a propagated memory chain
        PropagatedMemory* FindOrCreatePropagatedMemory(const IDStack& chain, PropagatedMemoryRange* range) {
            TrivialStackVector<MemoryAddressNode, 32> workingNodes;

            // First, try to find it
            if (PropagatedMemory* memory = FindPropagatedMemory(chain, range, workingNodes)) {
                return memory;
            }

            // Nothing found, create the memory
            PropagatedMemory* memory = range->values.emplace_back(blockAllocator.Allocate<PropagatedMemory>());
            memory->addressChain.count = static_cast<uint32_t>(chain.Size());
            memory->addressChain.nodes = blockAllocator.AllocateArray<MemoryAddressNode>(memory->addressChain.count);

            // Root tree
            MemoryAccessTreeNode* treeNode = &range->tree;

            // Copy nodes
            for (uint32_t i = 0; i < static_cast<uint32_t>(chain.Size()); i++) {
                memory->addressChain.nodes[i] = workingNodes[i];

                MemoryAccessTreeNode* nextTree = nullptr;

                // Find next link
                for (auto && tag : treeNode->children) {
                    if (tag.first == workingNodes[i]) {
                        nextTree = tag.second;
                        break;
                    }
                }

                // Create if not found
                if (!nextTree) {
                    nextTree = treeNode->children.emplace_back(
                        workingNodes[i],
                        new MemoryAccessTreeNode()
                    ).second;
                }

                treeNode = nextTree;
            }

            // Set node memory
            treeNode->memory = memory;

            // OK!
            return memory;
        }

        /// Get an address node for an identifier
        MemoryAddressNode GetMemoryAddressNode(ID id) {
            MemoryAddressNode node;

            // Constant?
            const PropagatedValue& value = propagationValues.at(id);

            // Set payload
            if (value.lattice == Backend::IL::PropagationResult::Mapped) {
                node.type = MemoryAddressType::Constant;
                node.constant = value.constant;
            } else {
                node.type = MemoryAddressType::Varying;
                node.varying = id;
            }

            // OK
            return node;
        }

    private:
        /// Composite all active memory ranges
        void CompositePropagatedMemoryRanges() {
            for (size_t i = 0; i < propagationValues.size(); i++) {
                PropagatedValue& value = propagationValues[i];

                // Nothing to composite to?
                if (!value.memory) {
                    continue;
                }

                CompositePropagatedMemoryRange(value, static_cast<ID>(i));
            }
        }

        /// Find the base memory node
        MemoryAccessTreeNode* FindBaseMemoryNode(MemoryAccessTreeNode* node) {
            for (auto && tag : node->children) {
                if (tag.first.type != MemoryAddressType::Constant) {
                    continue;
                }

                // Check for base offset
                auto _int = tag.first.constant->Cast<IntConstant>();
                if (!_int || _int->value != 0) {
                    continue;
                }

                return tag.second;
            }

            return nullptr;
        }

        /// Composite a known propagated memory range
        void CompositePropagatedMemoryRange(PropagatedValue& value, ID id) {
            ASSERT(!value.constant, "Memory range value with a pre-assigned constant");

            // Get type of constant
            const Backend::IL::Type* type = program.GetTypeMap().GetType(id);
            if (!type) {
                ASSERT(false, "Failed to map constant to pointer kind");
                return;
            }

            // Must be pointer type
            const auto* ptrType = type->Cast<Backend::IL::PointerType>();
            if (!ptrType) {
                ASSERT(false, "Non-indirect memory propagation range");
                return;
            }

            // Assume the lattice is mapped
            Backend::IL::PropagationResult lattice = Backend::IL::PropagationResult::Mapped;

            // Construct the constant
            const Constant* constant = CompositeConstant(ptrType->pointee, &value.memory->tree, lattice);
            if (constant == nullptr) {
                return;
            }

            // Confident about the values mapping
            value.lattice = lattice;
            value.constant = constant;
        }

        /// Composite a memory node to a constant
        const Constant* CompositeConstant(const Backend::IL::Type* type, MemoryAccessTreeNode* node, Backend::IL::PropagationResult& lattice) {
            switch (type->kind) {
                default: {
                    return nullptr;
                }
                case Backend::IL::TypeKind::Int:
                case Backend::IL::TypeKind::FP:
                case Backend::IL::TypeKind::Bool:
                    return CompositePrimitiveConstant(type, node, lattice);
                case Backend::IL::TypeKind::Array:
                    return CompositeConstant(type->As<Backend::IL::ArrayType>(), node, lattice);
            }
        }

        /// Composite a memory node to a constant
        const Constant* CompositePrimitiveConstant(const Backend::IL::Type* type, MemoryAccessTreeNode* node, Backend::IL::PropagationResult& lattice) {
            if (!node->memory) {
                return nullptr;
            }

            if (node->memory->lattice != Backend::IL::PropagationResult::Mapped) {
                lattice = Backend::IL::PropagationResult::Varying;
            }

            return node->memory->value;
        }

        /// Composite a memory node to a constant
        const Constant* CompositeConstant(const Backend::IL::ArrayType* type, MemoryAccessTreeNode* node, Backend::IL::PropagationResult& lattice) {
            ArrayConstant array;
            array.elements.resize(type->count);

            // Handle all mapped elements
            for (auto&& tag : node->children) {
                if (tag.first.type != MemoryAddressType::Constant) {
                    continue;
                }

                const auto* offset = tag.first.constant->As<IntConstant>();

                // Try to construct element
                array.elements.at(offset->value) = CompositeConstant(type->elementType, tag.second, lattice);
            }

            bool isSymbolic = false;

            // Check for partial constants
            for (const Constant* element : array.elements) {
                if (!element) {
                    lattice = Backend::IL::PropagationResult::Varying;
                }

                isSymbolic |= !element || element->IsSymbolic();
            }

            // If varying, create as a symbolic constant
            if (isSymbolic) {
                return program.GetConstants().AddSymbolicConstant(type, array);
            } else {
                return program.GetConstants().FindConstantOrAdd(type, array);
            }
        }

        /// Traverse an immediate constant from an id stack
        const Constant* TraverseImmediateConstant(const Constant* composite, const IDStack& stack) {
            for (size_t i = 0; i < stack.Size(); i++) {
                const PropagatedValue& value = propagationValues.at(stack[i]);

                // Unmapped?
                if (value.lattice != Backend::IL::PropagationResult::Mapped) {
                    return nullptr;
                }

                // Must be integer
                auto index = composite->Cast<IntConstant>();
                if (!index) {
                    return nullptr;
                }

                switch (composite->kind) {
                    default: {
                        // Unknown traversal
                        return nullptr;
                    }
                    case Backend::IL::ConstantKind::Array: {
                        auto array = value.constant->As<ArrayConstant>();
                        return array->elements.at(index->value);
                    }
                    case Backend::IL::ConstantKind::Struct: {
                        auto _struct = value.constant->As<StructConstant>();
                        return _struct->members.at(index->value);
                    }
                }
            }

            // OK
            return composite;
        }

        /// Create a memory tree from an immediate constant
        /// Useful for constant injection prior to propagation
        void CreateMemoryTreeFromImmediate(MemoryAccessTreeNode* node, const Constant* constant) {
            switch (constant->kind) {
                default: {
                    node->memory = new PropagatedMemory {
                        .lattice = Backend::IL::PropagationResult::Mapped,
                        .value = constant
                    };
                    break;
                }
                case Backend::IL::ConstantKind::Array: {
                    auto array = constant->As<ArrayConstant>();

                    for (uint32_t i = 0; i < array->elements.size(); i++) {
                        MemoryAddressNode addressNode {
                            .type = MemoryAddressType::Constant,
                            .constant = program.GetConstants().FindConstantOrAdd(
                                program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth = 32, .signedness = true}),
                                Backend::IL::IntConstant{.value = i}
                            )
                        };

                        auto&& tag = node->children.emplace_back(addressNode, new MemoryAccessTreeNode { });

                        CreateMemoryTreeFromImmediate(tag.second, array->elements[i]);
                    }
                    break;
                }
                case Backend::IL::ConstantKind::Struct: {
                    auto _struct = constant->As<StructConstant>();

                    for (uint32_t i = 0; i < _struct->members.size(); i++) {
                        MemoryAddressNode addressNode {
                            .type = MemoryAddressType::Constant,
                            .constant = program.GetConstants().FindConstantOrAdd(
                                program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth = 32, .signedness = true}),
                                Backend::IL::IntConstant{.value = i}
                            )
                        };

                        auto&& tag = node->children.emplace_back(addressNode, new MemoryAccessTreeNode { });

                        CreateMemoryTreeFromImmediate(tag.second, _struct->members[i]);
                    }
                    break;
                }
            }
        }

    private:
        /// TODO: Reduce allocations

        struct LoopInfo {
            /// Outer definition
            const Loop* definition{nullptr};

            /// All merged side effects of an iteration
            std::unordered_map<const PropagatedMemory*, PropagatedMemorySSAVersion> memoryLookup;
        };

        struct ReachingStoreResult {
            /// Result of the store search
            /// May be overdefined on ambiguous searches
            Backend::IL::PropagationResult result{Backend::IL::PropagationResult::None};

            /// Found version
            PropagatedMemorySSAVersion* version{nullptr};
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
        ReachingStoreResult FindReachingStoreDefinition(const BasicBlock* block, const Instruction* instr, const PropagatedMemory* memory) {
            ReachingStoreCache cache;
            return FindReachingStoreDefinition(block, instr, memory, cache);
        }

        /// Find the reaching, i.e., dominating, store definition with a matching memory tree
        ReachingStoreResult FindReachingStoreDefinition(const BasicBlock* block, const Instruction* instr, const PropagatedMemory* memory, ReachingStoreCache& cache) {
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
        ReachingStoreResult FindReachingStoreDefinitionInner(const BasicBlock* block, const Instruction* instr, const PropagatedMemory* memory, ReachingStoreCache& cache) {
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
                auto it = ssaMemoryLookup.find(blockInstr);
                if (it == ssaMemoryLookup.end() || it->second.memory != memory) {
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
            if (auto loopIt = loopLookup.find(block); loopIt != loopLookup.end()) {
                LoopInfo& info = loopIt->second;
                loopDefinition = info.definition;

                // Check if the memory pattern exists
                // Note that address checks on the loop memory ranges is fine, as it should be unique anyway
                if (auto memoryIt = info.memoryLookup.find(memory); memoryIt != info.memoryLookup.end()) {
                    result.result = Backend::IL::PropagationResult::Mapped;
                    result.version = &memoryIt->second;
                    return result;
                }
            }

            // None found, check predecessors
            const DominatorAnalysis::BlockView& predecessors = dominatorAnalysis.GetPredecessors(block);
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

    private:
        /// Outer program
        Program& program;

        /// Domination tree
        const DominatorAnalysis& dominatorAnalysis;

        /// Loop tree
        const LoopAnalysis& loopAnalysis;

        /// All users
        UserAnalysis users;

    private:
        /// All propagated values (result wise lookup)
        std::vector<PropagatedValue> propagationValues;

        /// Underlying propagation engine
        Backend::IL::PropagationEngine propagationEngine;

    private:
        /// Memory lookup for SSA instructions
        std::unordered_map<const Instruction*, PropagatedMemorySSAVersion> ssaMemoryLookup;

        /// Look lookup
        std::unordered_map<const BasicBlock*, LoopInfo> loopLookup;

    private:
        /// Block allocator for constants, constants never need to be freed
        LinearBlockAllocator<1024> blockAllocator;
    };
}
