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
#include <Backend/IL/Execution/ExecutionFlag.h>

/// Max number of marker hashes we track
static constexpr uint32_t kMaxExecutionInfoMarkerCount = 5;

struct ExecutionInfo {
    /// Rolling counter UIDs
    /// This is typically atomically allocated, and is expected to roll on the numerical limit
    /// Useful for expected transient results based on <reasonable> amounts of invocations within submissions
    uint32_t rollingExecutionUID;
    uint32_t rollingViewportUID;

    /// Execution set flags
    ExecutionFlagSet executionFlags;
    
    /// UID of the pipeline being executed
    uint32_t pipelineUID;

    /// Local hashes of markers
    uint32_t markerHashes32[kMaxExecutionInfoMarkerCount];

    /// UID of the queue this is currently executing on
    uint32_t queueUID;

    /// Payload data
    struct {
        struct {
            ExecutionDrawFlagSet drawFlags;
            
            /// Parameters
            uint32_t vertexCountPerInstance;
            uint32_t indexCountPerInstance;
            uint32_t instanceCount;
            uint32_t startVertex;
            uint32_t startIndex;
            uint32_t startInstance;
            uint32_t vertexOffset;
            uint32_t instanceOffset;
        } draw;

        struct {
            /// General dispatch dimensions
            uint32_t groupCountX;
            uint32_t groupCountY;
            uint32_t groupCountZ;
        } dispatch;
    };

    /// State data
    struct {
        struct {
            uint32_t width;
            uint32_t height;
        } viewport;
    };
};

/// Number of dwords required for the execution structure
static constexpr uint32_t kExecutionInfoDWordCount = sizeof(ExecutionInfo) / sizeof(uint32_t);

/// Sanity check
static_assert(sizeof(ExecutionInfo) == sizeof(uint32_t) * 24);
