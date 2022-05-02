#pragma once

// Backend
#include <Backends/DX12/DX12.h>

/// Hooks
ULONG WINAPI HookIDXGISwapChainRelease(IDXGISwapChain* swapChain);
