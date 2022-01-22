#pragma once

// Backend
#include "VisitContext.h"

// Common
#include <Common/Containers/TrivialStackVector.h>

namespace IL {
    /// Implementation details
    namespace Detail {
        template<typename F>
        bool VisitUserInstructions(IL::Program& program, IL::Function& function, IL::BasicBlock* basicBlock, F&& functor) {
            bool migratedBlock{false};

            // Visit all instructions
            for (auto instruction = basicBlock->begin(); instruction != basicBlock->end(); ++instruction) {
                if (!instruction->IsUserInstruction()) {
                    continue;
                }

                // Set up the context
                VisitContext context {
                        .program = program,
                        .function = function,
                        .basicBlock = *basicBlock,
                };

                // Pass through visitor
                instruction = functor(context, instruction);

                // Migrated block?
                if (instruction.block != basicBlock) {
                    basicBlock = instruction.block;

                    // Invalidate the parent iteration
                    migratedBlock = true;

                    // Mark as visited
                    basicBlock->AddFlag(BasicBlockFlag::Visited);
                }

                // Exit?
                if (context.flags & VisitFlag::Stop) {
                    return migratedBlock;
                }
            }

            return migratedBlock;
        }

        template<typename F>
        void VisitUserInstructions(IL::Program& program, IL::Function& function, F&& functor) {
            for (;;) {
                bool mutated{false};

                // Current revision
                uint32_t revision = function.GetBasicBlockRevision();

                // Visit all blocks
                for (IL::BasicBlock& basicBlock : function) {
                    // If instrumented or visited, skip
                    if (basicBlock.GetFlags() & (BasicBlockFlag::NoInstrumentation | BasicBlockFlag::Visited)) {
                        continue;
                    }

                    // Mark as visited
                    basicBlock.AddFlag(BasicBlockFlag::Visited);

                    // Visit all instructions
                    bool migratedBlock = VisitUserInstructions(program, function, &basicBlock, functor);

                    // If the fn revision has changed (block removed, moved, added), we need to re-visit
                    if (migratedBlock || revision != function.GetBasicBlockRevision()) {
                        // Mark as mutated
                        mutated = true;

                        // Stop!
                        break;
                    }
                }

                // Stop if mutated
                if (!mutated) {
                    break;
                }
            }
        }

        template<typename F>
        void VisitUserInstructions(IL::Program& program, F&& functor) {
            for (;;) {
                bool mutated{false};

                // Current revision
                uint32_t revision = program.GetFunctionRevision();

                // Visit all functions
                for (IL::Function& function : program) {
                    // If instrumented or visited, skip
                    if (function.GetFlags() & (FunctionFlag::NoInstrumentation | FunctionFlag::Visited)) {
                        continue;
                    }

                    // Visit all instructions
                    VisitUserInstructions(program, function, functor);

                    // Mark as visited
                    function.AddFlag(FunctionFlag::Visited);

                    // If the pg revision has changed (block removed, moved, added), we need to re-visit
                    if (revision != program.GetFunctionRevision()) {
                        // Mark as mutated
                        mutated = true;

                        // Stop!
                        break;
                    }
                }

                // Stop if mutated
                if (!mutated) {
                    break;
                }
            }
        }
    }

    /// Visit all user instructions within a program
    /// \param program the program to be traversed
    /// \param functor the instruction functor, type must be invocable with (VisitContext&, const BasicBlock::Iterator&) -> BasicBlock::Iterator,
    ///                 functor return type dictates the next visitation. Must be a valid iterator for the current basic block.
    template<typename F>
    void VisitUserInstructions(IL::Program& program, F&& functor) {
        // Visit all functions
        Detail::VisitUserInstructions(program, functor);

        // Clean visitation states
        for (IL::Function& function : program) {
            function.RemoveFlag(FunctionFlag::Visited);

            for (IL::BasicBlock &basicBlock: function) {
                basicBlock.RemoveFlag(BasicBlockFlag::Visited);
            }
        }
    }
}
