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

// Std
#include <cstdint>

/// Diagnostic logging on/off?
#ifndef NDEBUG
#   define DX12_DIAGNOSTIC 1
#else
#   define DX12_DIAGNOSTIC 0
#endif

/// Enable debugging mode for shader compiler
#define SHADER_COMPILER_DEBUG (DX12_DIAGNOSTIC && 0)

/// Enable serial compilation for debugging purposes
#define SHADER_COMPILER_SERIAL (DX12_DIAGNOSTIC && 0)

/// Log allocations
#define LOG_ALLOCATION (DX12_DIAGNOSTIC && 0)

/// Validates LLVM bit-stream 1:1 read / writes
#define DXIL_VALIDATE_MIRROR (DX12_DIAGNOSTIC && 0)

/// Dump DXIL bit stream to file?
///   Useful with llvm-bcanalyzer
#define DXIL_DUMP_BITSTREAM (DX12_DIAGNOSTIC && 0)

/// Dump DXBC byte stream to file?
#define DXBC_DUMP_STREAM (DX12_DIAGNOSTIC && 0)

/// Pretty print pipeline IL?
#define DXIL_PRETTY_PRINT (DX12_DIAGNOSTIC && 0)

/// Log instrumentation information
#define LOG_INSTRUMENTATION (1)

/// Log instrumentation keys that have been rejected
#define LOG_REJECTED_KEYS (1)

/// Enable the experimental shading models for signing bypass
#define USE_EXPERIMENTAL_SHADING_MODELS (1)

/// Enable the inbuilt DXBC to DXIL conversion, experimental
#define USE_DXBC_TO_DXIL_CONVERSION (1)

/// Enables the DXBC signer, as DXBC support is done through DXIL, this is not required
#define USE_DXBC_SIGNER (0)

/// Enable instrumentation of a specific file for debugging purposes
///  ? Instrumentation of large applications can be difficult to debug and even harder to reproduce under the same conditions.
///    When such a fault occurs, it is very useful to simply be able to iterate on a binary file.
#define SHADER_COMPILER_DEBUG_FILE (DX12_DIAGNOSTIC && 0)

/** Options **/

/// Prefix all injected descriptors
/// |-GRS-|-USER-SPACE-| 
#define DESCRIPTOR_HEAP_METHOD_PREFIX  0

/// Postfix all injected descriptors
/// |-USER-SPACE-|-GRS-| 
#define DESCRIPTOR_HEAP_METHOD_POSTFIX 1

/// Current heap method
#define DESCRIPTOR_HEAP_METHOD DESCRIPTOR_HEAP_METHOD_POSTFIX

/** Constants **/

/// Maximum number of dwords in a root signature
static constexpr uint32_t MaxRootSignatureDWord = 64;
