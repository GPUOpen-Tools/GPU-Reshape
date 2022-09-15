#pragma once

/// Diagnostic logging on/off?
#ifndef NDEBUG
#   define VK_DIAGNOSTIC 1
#else
#   define VK_DIAGNOSTIC 0
#endif

/// Log allocations
#define LOG_ALLOCATION (VK_DIAGNOSTIC && 0)

/// Log instrumentation information
#define LOG_INSTRUMENTATION (VK_DIAGNOSTIC)

/// Log instrumentation keys that have been rejected
#define LOG_REJECTED_KEYS (VK_DIAGNOSTIC)

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
