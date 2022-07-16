#pragma once

/// Diagnostic logging on/off?
#ifndef NDEBUG
#   define DX12_DIAGNOSTIC 1
#else
#   define DX12_DIAGNOSTIC 0
#endif

/// Enable debugging mode for shader compiler
#define SHADER_COMPILER_DEBUG 1
