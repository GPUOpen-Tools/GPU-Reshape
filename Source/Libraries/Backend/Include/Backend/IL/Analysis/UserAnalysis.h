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

// Std
#include <vector>
#include <span>

namespace IL {
    class UserAnalysis : public IProgramAnalysis {
    public:
        COMPONENT(UserAnalysis);
        
        /// Users are represented as instruction references, as users may include instructions without a result
        using UserView = std::span<const ConstInstructionRef<>>;

        UserAnalysis(Program& program) : program(program) {
            /* poof */
        }

        bool Compute() {
            // Allocate all views
            views.resize(program.GetIdentifierMap().GetMaxID(), {nullptr, 0});

            // Total number of users
            uint32_t userCount = 0;

            // Count all number of users per instruction
            for (const Function* function : program.GetFunctionList()) {
                for (const BasicBlock* block : function->GetBasicBlocks()) {
                    for (auto instrIt = block->begin(); instrIt != block->end(); ++instrIt) {
                        Backend::IL::VisitOperands(instrIt.Get(), [&](IL::ID operand) {
                            views[operand].size++;
                            userCount++;
                        });
                    }
                }
            }

            // Allocate all users, linear array but is chunked out of
            identifiers.resize(userCount);

            // Local offset
            uint32_t userOffset = 0;

            // Write all users
            for (const Function* function : program.GetFunctionList()) {
                for (const BasicBlock* block : function->GetBasicBlocks()) {
                    for (auto instrIt = block->begin(); instrIt != block->end(); ++instrIt) {
                        const Instruction* ptr = instrIt.Get();

                        Backend::IL::VisitOperands(ptr, [&](IL::ID operand) {
                            UserViewData& data = views[operand];

                            // If there's no user data yet, allocate it
                            if (!data.view) {
                                data.view = identifiers.data() + userOffset;
                                userOffset += data.size;
                            }

                            // Write operand, i.e. mark this instruction as a user for the operand source
                            data.view[data.head++] = instrIt.Ref();
                        });
                    }
                }
            }

            // Validate allocation range
            ASSERT(userCount == userOffset, "Invalid user offset");

            // Validate user write range
#ifndef NDEBUG
            for (const UserViewData& view : views) {
                ASSERT(view.head == view.size, "Invalid head to size");
            }
#endif // NDEBUG

            // OK
            return true;
        }

        /// Get all users for an id
        UserView GetUsers(IL::ID id) const {
            return UserView(views[id].view, views[id].size);
        }

    private:
        struct UserViewData {
            IL::ConstInstructionRef<>* view{nullptr};
            uint32_t head{0};
            uint32_t size{0};
        };

    private:
        /// Outer program
        Program& program;

        /// All views
        std::vector<UserViewData> views;

        /// All identifiers, chunked out of
        std::vector<IL::ConstInstructionRef<>> identifiers;
    };
}
