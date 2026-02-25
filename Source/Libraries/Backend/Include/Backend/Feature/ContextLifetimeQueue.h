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

#pragma once

// Backend
#include <Backend/CommandContextHandle.h>
#include <Backend/SubmissionContext.h>
#include <Backend/Scheduler/IScheduler.h>

// Common
#include <Common/ComRef.h>

// Std
#include <cstdint>
#include <unordered_map>

class ContextLifetimeQueue {
public:
    /// Install this lifetime queue
    /// @param scheduler the device scheduler
    void Install(const ComRef<IScheduler>& scheduler) {
        primitiveID = scheduler->CreatePrimitive();
    }

    /// Join a handle, must only be invoked on completion
    /// @param handle handle to join
    void Join(CommandContextHandle handle) {
        auto it = pendingContexts.find(handle);
        if (it == pendingContexts.end()) {
            return;
        }

        // Linear remove the pending commit, should be fast since it's sorted
        pendingCommits.erase(
            std::remove(pendingCommits.begin(), pendingCommits.end(), it->second),
            pendingCommits.end()
        );

        // OK
        pendingContexts.erase(it);
    }

    /// Enqueue a context and its handles
    /// @param submitContext submit context
    /// @param contexts all contexts being submitted
    /// @param contextCount number of contexts
    void Enqueue(SubmissionContext& submitContext, const CommandContextHandle* contexts, uint32_t contextCount) {
        // Pre-increment, since "this" is the last submitted value
        uint64_t value = ++commitHead;

        // Associate all the contexts
        for (uint32_t i = 0; i < contextCount; i++) {
            pendingContexts[contexts[i]] = value;
        }

        // Signal the counter
        submitContext.signalPrimitives.Add(SchedulerPrimitiveEvent {
            .id = primitiveID,
            .value = value
        });

        // Insert, always "sorted"
        pendingCommits.push_back(value);
    }

    /// Get the current commit head
    uint64_t GetCommitHead() {
        return commitHead;
    }

    /// Check if a value is committed
    uint64_t IsCommitted(uint64_t head) {
        return 
            pendingContexts.empty() || 
            pendingCommits.empty() || 
            head < pendingCommits.front();
    }

private:
    /// All live commits
    std::vector<uint64_t> pendingCommits;

    /// All pending contexts and the commit values
    std::unordered_map<CommandContextHandle, uint64_t> pendingContexts;

    /// Underlying primitive for signalling
    SchedulerPrimitiveID primitiveID{InvalidSchedulerPrimitiveID};

    /// Current primitive value
    uint64_t commitHead{0};
};
