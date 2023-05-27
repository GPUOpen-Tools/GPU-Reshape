#include <Backend/IL/Function.h>
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/PrettyPrint.h>
#include <Backend/IL/CFG/DominatorTree.h>

// Common
#include <Common/Containers/BitArray.h>
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

    // Always mark the entry point as acquired
    basicBlocks.Add(entryPoint);
    blockAcquiredArray[entryPoint->GetID()] = true;

    // Resolve loop
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
