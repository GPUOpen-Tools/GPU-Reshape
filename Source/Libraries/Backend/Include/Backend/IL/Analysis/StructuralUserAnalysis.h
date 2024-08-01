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
#include <Backend/IL/Analysis/IAnalysis.h>
#include <Backend/IL/Analysis/UserAnalysis.h>
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/Program.h>

// Common
#include <Common/Sink.h>
#include <Common/Containers/BitArray.h>

// Std
#include <vector>

namespace IL {
    class StructuralUserAnalysis : public IProgramAnalysis {
    public:
        COMPONENT(StructuralUserAnalysis);
        
        StructuralUserAnalysis(Program& program) : program(program) {
            /* poof */
        }

        /// Compute this pass
        /// \return success state
        bool Compute() {
            // Get or compute user analysis
            userAnalysis = program.GetAnalysisMap().FindPassOrCompute<UserAnalysis>(program);

            // Initialize local views
            views.resize(program.GetIdentifierMap().GetMaxID());

            // Handle all instructions
            for (const Function* function : program.GetFunctionList()) {
                for (const BasicBlock* block : function->GetBasicBlocks()) {
                    for (auto instrIt = block->begin(); instrIt != block->end(); ++instrIt) {
                        switch (instrIt->opCode) {
                            default: {
                                // If this is not an instruction that performs sub-addressing, any operand
                                // usage of a view value is treated as a <full> structural usage
                                Backend::IL::VisitOperands(instrIt.Get(), [&](IL::ID operand) {
                                    const Backend::IL::Type* type = program.GetTypeMap().GetType(operand);
                                    if (!type) {
                                        return;
                                    }

                                    // Get underlying dimensionality
                                    uint32_t dimension = GetTypeDimension(type);

                                    // More than stack path?
                                    if (dimension >= 64) {
                                        views[operand].lowIndices = ~0ull;
                                        views[operand].upperIndices.Resize(dimension - 64);

                                        for (uint32_t i = 64; i < dimension; i++) {
                                            views[operand].upperIndices[i] = true;
                                        }
                                    } else {
                                        views[operand].lowIndices = (1u << dimension) - 1;
                                    }
                                });
                                break;
                            }
                            case OpCode::Extract: {
                                auto _instr = instrIt->As<ExtractInstruction>();

                                // Only track first chain
                                const Constant* constant = program.GetConstants().GetConstant(_instr->chains[0].index);
                                if (!constant) {
                                    continue;
                                }

                                MarkAsUsed(_instr->composite, static_cast<uint32_t>(constant->As<IntConstant>()->value));
                                break;
                            }
                            case OpCode::AddressChain: {
                                auto _instr = instrIt->As<AddressChainInstruction>();

                                // Only track first chain
                                const Constant* constant = program.GetConstants().GetConstant(_instr->chains[0].index);
                                if (!constant) {
                                    continue;
                                }

                                MarkAsUsed(_instr->composite, static_cast<uint32_t>(constant->As<IntConstant>()->value));
                                break;
                            }
                        }
                    }
                }
            }

            // OK
            return true;
        }

        /// Mark a value as used
        /// \param id id to mark as used
        /// \param index structural index to mark as used
        void MarkAsUsed(IL::ID id, uint32_t index) {
            StructuralEntry& entry = views[id];

            // Stack path?
            if (index < 64) {
                entry.lowIndices |= 1ull << index;
            } else {
                if (index >= entry.upperIndices.Size()) {
                    entry.upperIndices.Resize(index + 1);
                }
                                    
                entry.upperIndices[index] = true;
            }
        }

        /// Get number of used indices of a value
        /// \param id given value
        /// \return index count
        uint32_t GetUsedIndexCount(IL::ID id) {
            return static_cast<uint32_t>(std::popcount(views[id].lowIndices) + views[id].upperIndices.PopCount());
        }

        /// Check if an index is used on a value
        /// \param id given value
        /// \param index index to check for
        /// \return true if used
        bool IsIndexUsed(IL::ID id, uint32_t index) {
            // Stack path?
            if (index < 64) {
                return views[id].lowIndices & (1ull << index);
            }
            
            if (index >= views[id].upperIndices.Size()) {
                return false;
            }

            return views[id].upperIndices[index];
        }

        /// Reinterpret the structural usage as a component mask
        /// \param id given id to fetch a component mask for
        /// \return component mask
        ComponentMask GetUsedComponentMask(IL::ID id) const {
            return static_cast<ComponentMask>(views[id].lowIndices & 0b1111);
        }

        /// Get the shared user analysis
        const ComRef<UserAnalysis>& GetUserAnalysis() const {
            return userAnalysis;
        }

    private:
        struct StructuralEntry {
            // Stack path
            uint64_t lowIndices{0};

            // Heap path (>64)
            BitArray upperIndices;
        };

    private:
        /// Outer program
        Program& program;

        /// All views
        std::vector<StructuralEntry> views;

        /// Shared user analysis
        ComRef<UserAnalysis> userAnalysis;
    };
}
