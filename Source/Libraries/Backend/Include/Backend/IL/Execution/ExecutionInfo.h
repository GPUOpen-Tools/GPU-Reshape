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

struct ExecutionInfo {
    /// A rolling execution counter UID
    /// This is typically atomically allocated, and is expected to roll on the numerical limit
    /// Useful for expected transient results based on <reasonable> amounts of invocations within submissions
    uint32_t rollingExecutionUID;

    /// Execution set flags
    ExecutionFlagSet executionFlags;
    
    /// UID of the pipeline being executed
    uint32_t pipelineUID;

    /// UID of the active scope
    uint32_t scopeUID;

    /// Payload data
    struct {
        struct {
            /// Number of vertices
            uint32_t vertexCount;

            /// Number of indices
            uint32_t indexCount;
        } draw;

        struct {
            /// General dispatch dimensions
            uint32_t groupCountX;
            uint32_t groupCountY;
            uint32_t groupCountZ;
        } dispatch;
    } payload;
};

/// Number of dwords required for the execution structure
static constexpr uint32_t kExecutionInfoDWordCount = sizeof(ExecutionInfo) / sizeof(uint32_t);

/// Sanity check
static_assert(sizeof(ExecutionInfo) == sizeof(uint32_t) * 9);
