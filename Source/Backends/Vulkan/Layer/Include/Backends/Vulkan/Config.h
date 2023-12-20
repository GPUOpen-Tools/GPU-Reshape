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

/// Diagnostic logging on/off?
#ifndef NDEBUG
#   define VK_DIAGNOSTIC 1
#else // NDEBUG
#   define VK_DIAGNOSTIC 0
#endif // NDEBUG

/** Debugging **/

/// Log allocations
#define LOG_ALLOCATION (VK_DIAGNOSTIC && 0)

/// Log instrumentation information
#define LOG_INSTRUMENTATION (1)

/// Log instrumentation keys that have been rejected
#define LOG_REJECTED_KEYS (1)

/// Disables pipeline batching for debugging purposes
#define PIPELINE_COMPILER_NO_BATCH (VK_DIAGNOSTIC && 0)

/// Enable serial compilation for debugging purposes
#define SHADER_COMPILER_SERIAL (VK_DIAGNOSTIC && 0)

/// Enable instrumentation of a specific file for debugging purposes
///  ? Instrumentation of large applications can be difficult to debug and even harder to reproduce under the same conditions.
///    When such a fault occurs, it is very useful to simply be able to iterate on a spv binary file.
#define SHADER_COMPILER_DEBUG_FILE (VK_DIAGNOSTIC && 0)

/// Enable debugging mode for shader compiler
#define SHADER_COMPILER_DEBUG (VK_DIAGNOSTIC && 0)

/// Enable tracking of bound descriptor sets for debugging
#define TRACK_DESCRIPTOR_SETS (VK_DIAGNOSTIC && 0)

/** Options **/

/// Use dynamic uniform buffer objects for PRMT binding
#define PRMT_METHOD_UB_DYNAMIC 0

/// Use uniform buffer objects with push constant offsets for PRMT binding
#define PRMT_METHOD_UB_PC  1

/// Current PRMT method
#define PRMT_METHOD PRMT_METHOD_UB_PC

/// Merge all push-constant ranges, this greatly simplifies mappings
///   Push constant ranges may be overlapping, however, the mappings are contiguous host side.
///   An alternative implementations would extend each staged bit, and then append new stages if
///   needed. However, this complicates the user side updates.
#define PIPELINE_MERGE_PC_RANGES 1
