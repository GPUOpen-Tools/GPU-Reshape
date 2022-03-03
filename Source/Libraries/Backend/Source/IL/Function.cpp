#include <Backend/IL/Function.h>
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/PrettyPrint.h>

// Std
#ifndef NDEBUG
#include <sstream>
#endif

// System
#if defined(_MSC_VER)
#   include <Windows.h>
#   undef max
#endif

static void AddSuccessor(std::map<IL::ID, uint32_t>& userMap, IL::BasicBlockList &basicBlocks, IL::BasicBlock& block, IL::ID successor) {
    IL::BasicBlock* successorBlock = basicBlocks.GetBlock(successor);
    ASSERT(successorBlock, "Successor block invalid");

    // Must have terminator
    auto terminator = successorBlock->GetTerminator();
    ASSERT(terminator, "Must have terminator");

    // Get control flow, if present
    IL::BranchControlFlow controlFlow;
    switch (terminator.GetOpCode()) {
        default:
            break;
        case IL::OpCode::BranchConditional:
            controlFlow = terminator.As<IL::BranchConditionalInstruction>()->controlFlow;
            break;
    }

    // Skip loop back continue block for order resolving
    if (controlFlow._continue == block.GetID()) {
        return;
    }

    userMap[successor]++;
}

bool IL::Function::ReorderByDominantBlocks() {
    // Block user counters
    std::map<IL::ID, uint32_t> userMap;

    // Accumulate users
    for (IL::BasicBlock& block : basicBlocks) {
        auto terminator = block.GetTerminator();
        ASSERT(terminator, "Must have terminator");

        switch (terminator.GetOpCode()) {
            default:
                break;
            case IL::OpCode::Branch:
                AddSuccessor(userMap, basicBlocks, block, terminator.As<IL::BranchInstruction>()->branch);
                break;
            case IL::OpCode::BranchConditional:
                AddSuccessor(userMap, basicBlocks, block, terminator.As<IL::BranchConditionalInstruction>()->pass);
                AddSuccessor(userMap, basicBlocks, block, terminator.As<IL::BranchConditionalInstruction>()->fail);
                break;
        }
    }

    std::list<IL::BasicBlock> blocks;
    basicBlocks.SwapBlocks(blocks);

    // Mutation loop
    for (;;) {
        bool mutated = false;

        // Find candidate
        for (auto it = blocks.begin(); it != blocks.end(); it++) {
            // Find first with free users
            uint32_t users = userMap[it->GetID()];
            if (users) {
                continue;
            }

            // Must have terminator
            auto terminator = it->GetTerminator();
            ASSERT(terminator, "Must have terminator");

            // Remove users
            switch (terminator.GetOpCode()) {
                default:
                    break;
                case IL::OpCode::Branch:
                    userMap[terminator.As<IL::BranchInstruction>()->branch]--;
                    break;
                case IL::OpCode::BranchConditional:
                    userMap[terminator.As<IL::BranchConditionalInstruction>()->pass]--;
                    userMap[terminator.As<IL::BranchConditionalInstruction>()->fail]--;
                    break;
            }

            // Move block back to function
            basicBlocks.Add(std::move(*it));
            blocks.erase(it);

            // Mark as mutated
            mutated = true;

            // Reach around
            break;
        }

        // If no more mutations, stop
        if (!mutated) {
            break;
        }
    }

    // Must have moved all
    if (!blocks.empty()) {
        // Move unresolved blocks black
        for (IL::BasicBlock& block : blocks) {
            basicBlocks.Add(std::move(block));
        }

#ifndef NDEBUG
        // Pretty print the blocks
        std::stringstream ss;
        IL::PrettyPrint(*this, IL::PrettyPrintContext(ss));

#ifdef _MSC_VER
        OutputDebugString(ss.str().c_str());
#endif

        fprintf(stderr, "%s\n", ss.str().c_str());
#endif

        ASSERT(false, "Failed to reorder dominant blocks");
        return false;
    }

    // OK
    return true;
}
