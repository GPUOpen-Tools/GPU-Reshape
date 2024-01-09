// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
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

#include <Backend/IL/Function.h>
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/PrettyPrint.h>
#include <Backend/IL/CFG/DominatorTree.h>
#include <Backend/IL/InstructionCommon.h>

// Common
#include <Common/Containers/BitArray.h>
#include <Common/Containers/TrivialStackVector.h>
#include <Common/FileSystem.h>

// Std
#include <unordered_set>
#ifndef NDEBUG
#include <sstream>
#include <fstream>
#include <mutex>
#endif

// System
#if defined(_MSC_VER)
#   include <Windows.h>
#   undef max
#endif

bool IL::Function::ReorderByDominantBlocks(bool hasControlFlow) {
    IL::BasicBlock* entryPoint = basicBlocks.GetEntryPoint();

    // Construct dominance tree from current basic blocks
    DominatorTree dominatorTree(GetBasicBlocks());
    dominatorTree.Compute();

    // Block acquisition lookup
    BitArray blockAcquiredArray(basicBlocks.GetBlockBound());

    // Swap local blocks
    BasicBlockList::Container blocks;
    basicBlocks.SwapBlocks(blocks);

    // Programs with control flow have slightly different requirements
    if (hasControlFlow) {
        // Standard depth-first traversal
        TrivialStackVector<IL::BasicBlock*, 64u> traversalStack;

        // Start with entry point
        blockAcquiredArray[entryPoint->GetID()] = true;
        traversalStack.Add(entryPoint);

        // Handle all pending blocks
        while (traversalStack.Size() > 0) {
            BasicBlock* basicBlock = traversalStack.PopBack();

            // Accept as resolved
            basicBlocks.Add(basicBlock);

            // Get termination control flow
            BranchControlFlow controlFlow;
            Backend::IL::GetControlFlow(basicBlock->GetTerminator(), controlFlow);

            // Merge must be evaluated after all successors within the construct has been evaluated
            if (controlFlow._continue != IL::InvalidID && blockAcquiredArray.Acquire(controlFlow._continue)) {
                ENSURE(traversalStack.Add(dominatorTree.GetBlock(controlFlow._continue)), "Failed to push block");
            }
            
            // Continue must be evaluated after all successors within the construct has been evaluated
            if (controlFlow.merge != IL::InvalidID && blockAcquiredArray.Acquire(controlFlow.merge)) {
                ENSURE(traversalStack.Add(dominatorTree.GetBlock(controlFlow.merge)), "Failed to push block");
            }

            // Visit all successors
            for (BasicBlock* successor : dominatorTree.GetSuccessors(basicBlock)) {
                if (blockAcquiredArray.Acquire(successor->GetID())) {
                    ENSURE(traversalStack.Add(successor), "Failed to push block");
                }
            }
        }
        
        // Append unreachable blocks
        for (BasicBlock* basicBlock : blocks) {
            if (!blockAcquiredArray.Acquire(basicBlock->GetID())) {
                continue;
            }

            // If truly unreachable, there's no predecessors
            ASSERT(dominatorTree.GetPredecessors(basicBlock).empty(), "Unappended block was reachable");

            // If truly unreachable, and by SPIRV standard, the successors are already present
            for (BasicBlock* successor : dominatorTree.GetSuccessors(basicBlock)) {
                ASSERT(!blockAcquiredArray[successor->GetID()], "Successor to unreachable not appended");
            }
            
            // Accept as resolved
            basicBlocks.Add(basicBlock);
        }
    } else {
        // Always mark the entry point as acquired
        basicBlocks.Add(entryPoint);
        blockAcquiredArray[entryPoint->GetID()] = true;
        
        // No control flow, resolve by immediate dominators
        for (size_t resolvedBlocks = 1; resolvedBlocks != blocks.size();) {
            bool mutated = false;

            // Visit all non-acquired blocks
            for (IL::BasicBlock* basicBlock : blocks) {
                if (blockAcquiredArray[basicBlock->GetID()]) {
                    continue;
                }

                // If the immediate dominator has been pushed, or there's none available (unreachable)
                BasicBlock* immediateDominator = dominatorTree.GetImmediateDominator(basicBlock);
                if (immediateDominator && !blockAcquiredArray[immediateDominator->GetID()]) {
                    continue;
                }

                // Acquire block
                blockAcquiredArray[basicBlock->GetID()] = true;
                basicBlocks.Add(basicBlock);

                // Mutate
                resolvedBlocks++;
                mutated = true;
            }

            // If no mutation, exit
            if (!mutated) {
                break;
            }
        }
    }
    
    // Must have moved all
    if (blocks.size() != basicBlocks.GetBlockCount()) {
#ifndef NDEBUG
        // Ensure crash is serial
        static std::mutex mutex;
        std::lock_guard guard(mutex);
#endif

        // Move unresolved blocks black
        for (IL::BasicBlock *block: blocks) {
            if (blockAcquiredArray[block->GetID()]) {
                continue;
            }

            basicBlocks.Add(block);
        }

#ifndef NDEBUG
        {
            // Pretty print the blocks
            std::stringstream ss;
            IL::PrettyPrint(*this, IL::PrettyPrintContext(ss));

#ifdef _MSC_VER
            OutputDebugString(ss.str().c_str());
#endif

            fprintf(stderr, "%s\n", ss.str().c_str());
        };
#endif

#ifndef NDEBUG
        {
            std::filesystem::path path = GetIntermediatePath("Crash");

            // Pretty print the graph
            std::ofstream out(path / "fn.graph.txt");
            IL::PrettyPrintBlockDotGraph(*this, IL::PrettyPrintContext(out));
            out.close();

            // Pretty print the graph
            std::ofstream outIL(path / "fn.il.txt");
            IL::PrettyPrint(*this, IL::PrettyPrintContext(outIL));
            outIL.close();

            // Toolset path
            std::filesystem::path graphVizDir = GetBaseModuleDirectory() / "GraphViz";

            // Check if the toolset is available
            if (std::filesystem::exists(graphVizDir)) {
#ifdef _MSC_VER
                // Startup information
                STARTUPINFO startupInfo;
                ZeroMemory(&startupInfo, sizeof(startupInfo));

                // Populate
                startupInfo.cb = sizeof(startupInfo);

                // Process information
                PROCESS_INFORMATION processInfo;
                ZeroMemory(&processInfo, sizeof(processInfo));

                // Executable
                std::string dotPath = (graphVizDir / "dot.exe").string();
                std::string arg     = "";

                // Output
                arg += " -Tpng";
                arg += " -o" + (path / "fn.graph.png").string();

                // Input
                arg += " " + (path / "fn.graph.txt").string();

                // Run graph viz
                CreateProcess(
                    dotPath.c_str(),
                    arg.data(),
                    NULL,
                    NULL,
                    FALSE,
                    0,
                    NULL,
                    NULL,
                    &startupInfo,
                    &processInfo
                );

                CloseHandle(processInfo.hProcess);
                CloseHandle(processInfo.hThread);
#endif
            }
        };
#endif

        ASSERT(false, "Failed to reorder dominant blocks");
        return false;
    }

    // OK
    return true;
}
