#pragma once

/// Diagnostic logging on/off?
#ifndef NDEBUG
#   define DX12_DIAGNOSTIC 1
#else
#   define DX12_DIAGNOSTIC 0
#endif

/// Enable debugging mode for shader compiler
#define SHADER_COMPILER_DEBUG (DX12_DIAGNOSTIC && 1)

/// Enable serial compilation for debugging purposes
#define SHADER_COMPILER_SERIAL (DX12_DIAGNOSTIC && 1)

/// Log allocations
#define LOG_ALLOCATION (DX12_DIAGNOSTIC && 0)

/// Validates LLVM bit-stream 1:1 read / writes
#define DXIL_VALIDATE_MIRROR (DX12_DIAGNOSTIC && 1)

/// Dump DXIL bit stream to file?
///   Useful with llvm-bcanalyzer
#define DXIL_DUMP_BITSTREAM (DX12_DIAGNOSTIC && 1)

/// Dump DXBC byte stream to file?
#define DXBC_DUMP_STREAM (DX12_DIAGNOSTIC && 1)

/// Pretty print pipeline IL?
#define DXIL_PRETTY_PRINT (DX12_DIAGNOSTIC && 1)

/// Log instrumentation information
#define LOG_INSTRUMENTATION (1)

/// Log instrumentation keys that have been rejected
#define LOG_REJECTED_KEYS (1)

/// Enable instrumentation of a specific file for debugging purposes
///  ? Instrumentation of large applications can be difficult to debug and even harder to reproduce under the same conditions.
///    When such a fault occurs, it is very useful to simply be able to iterate on a binary file.
#define SHADER_COMPILER_DEBUG_FILE (DX12_DIAGNOSTIC && 0)
