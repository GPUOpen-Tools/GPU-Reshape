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
#include <Backend/IL/InstructionAddressCommon.h>

namespace IL {
    class ConstantPropagatorMemory {
    public:
        /// Constructor
        /// \param program program to inject constants to
        ConstantPropagatorMemory(Program& program) : program(program) {
            
        }
        
        /// Initialize this memory host
        bool Install() {
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

                switch (variable->initializer->type->kind) {
                    default:
                        value.constant = variable->initializer;
                        break;
                    case Backend::IL::TypeKind::Struct:
                    case Backend::IL::TypeKind::Array:
                    case Backend::IL::TypeKind::Vector:
                        CreateMemoryTree(&GetMemoryRange(value)->tree, variable->initializer);
                        break;
                }

                propagationValues[variable->id] = value;
            }

            // OK
            return true;
        }

        void CompositeRanges() {
            // Finally, composite all memory ranges back into the typical constant layout
            CompositePropagatedMemoryRanges();
        }

    public:
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

        struct PropagatedMemoryTraversal {
            // Full match, may be null
            MemoryAccessTreeNode* match{nullptr};

            // Partial match on misses
            MemoryAccessTreeNode* partialMatch{nullptr};
        };

        struct LocalSSAMemory {
            /// Memory lookup for SSA instructions
            std::unordered_map<const Instruction*, PropagatedMemorySSAVersion> lookup;
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
        bool IsBaseOffsetNonConstantZero(ID id) const {
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
        ID PopulateAccessChain(ID id, IDStack& chain) const {
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
        PropagatedMemoryTraversal FindPropagatedMemory(const IDStack& chain, PropagatedMemoryRange* range) {
            MemoryAddressNodeStack ignore;
            return FindPropagatedMemory(chain, range, ignore);
        }

        /// Find the propagated memory
        PropagatedMemoryTraversal FindPropagatedMemory(const IDStack& chain, PropagatedMemoryRange* range, MemoryAddressNodeStack& workingNodes) {
            workingNodes.Resize(chain.Size());

            // Fill working nodes
            for (uint32_t i = 0; i < chain.Size(); i++) {
                workingNodes[i] = GetMemoryAddressNode(chain[i]);
            }

            // Root tree
            MemoryAccessTreeNode* treeNode = &range->tree;

            PropagatedMemoryTraversal out;

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
                    if (treeNode->memory) {
                        out.partialMatch = treeNode;
                    }
                    return out;
                }

                treeNode = nextTree;
            }

            if (treeNode->memory) {
                out.match = treeNode;
            }
            
            // OK
            return out;
        }

        /// Create a memory tree
        /// \param node target node to create from
        /// \param constant constant to propagate at target
        void CreateMemoryTree(MemoryAccessTreeNode* node, const Constant* constant) {
            switch (constant->type->kind) {
                default:
                    break;
                case Backend::IL::TypeKind::Struct:
                case Backend::IL::TypeKind::Array:
                case Backend::IL::TypeKind::Vector:
                    CreateMemoryTreeFromImmediate(node, constant);
                    break;
            }
        }

        /// Find or create a propagated memory chain
        MemoryAccessTreeNode* FindOrCreatePropagatedMemory(const IDStack& chain, PropagatedMemoryRange* range) {
            TrivialStackVector<MemoryAddressNode, 32> workingNodes;

            // First, try to find it
            if (PropagatedMemoryTraversal traversal = FindPropagatedMemory(chain, range, workingNodes); traversal.match) {
                return traversal.match;
            }

            // Nothing found, create the memory
            PropagatedMemory* memory = blockAllocator.Allocate<PropagatedMemory>();
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
            return treeNode;
        }

        /// Get an address node for an identifier
        MemoryAddressNode GetMemoryAddressNode(ID id) const {
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
            if (node->memory && node->memory->value && node->memory->value->Is<UnexposedConstant>()) {
                return node->memory->value;
            }
            
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
                case Backend::IL::TypeKind::Vector:
                    return CompositeConstant(type->As<Backend::IL::VectorType>(), node, lattice);
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

                // Try to construct element
                const auto* offset = tag.first.constant->As<IntConstant>();
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

        /// Composite a memory node to a constant
        const Constant* CompositeConstant(const Backend::IL::VectorType* type, MemoryAccessTreeNode* node, Backend::IL::PropagationResult& lattice) {
            VectorConstant array;
            array.elements.resize(type->dimension);

            // Handle all mapped elements
            for (auto&& tag : node->children) {
                if (tag.first.type != MemoryAddressType::Constant) {
                    continue;
                }

                // Try to construct element
                const auto* offset = tag.first.constant->As<IntConstant>();
                array.elements.at(offset->value) = CompositeConstant(type->containedType, tag.second, lattice);
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
                    case Backend::IL::ConstantKind::Vector: {
                        auto _struct = value.constant->As<VectorConstant>();
                        return _struct->elements.at(index->value);
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
                case Backend::IL::ConstantKind::Vector: {
                    auto _struct = constant->As<VectorConstant>();

                    for (uint32_t i = 0; i < _struct->elements.size(); i++) {
                        MemoryAddressNode addressNode {
                            .type = MemoryAddressType::Constant,
                            .constant = program.GetConstants().FindConstantOrAdd(
                                program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth = 32, .signedness = true}),
                                Backend::IL::IntConstant{.value = i}
                            )
                        };

                        auto&& tag = node->children.emplace_back(addressNode, new MemoryAccessTreeNode { });

                        CreateMemoryTreeFromImmediate(tag.second, _struct->elements[i]);
                    }
                    break;
                }
            }
        }
        
        /// All propagated values (result wise lookup)
        std::vector<PropagatedValue> propagationValues;

    private:
        /// Outer program
        Program& program;

        /// Block allocator for constants, constants never need to be freed
        LinearBlockAllocator<1024> blockAllocator;
    };
}
