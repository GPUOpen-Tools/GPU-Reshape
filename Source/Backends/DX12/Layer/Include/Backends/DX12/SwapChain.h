#pragma once

// Backend
#include <Backends/DX12/DX12.h>

/// Hooks
HRESULT WINAPI HookIDXGIFactoryCreateSwapChain(IDXGIFactory* factory, IUnknown *pDevice, DXGI_SWAP_CHAIN_DESC *pDesc, IDXGISwapChain **ppSwapChain);
HRESULT WINAPI HookIDXGIFactoryCreateSwapChainForHwnd(IDXGIFactory* factory, IUnknown *pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1 *pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc, IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain);
HRESULT WINAPI HookIDXGIFactoryCreateSwapChainForCoreWindow(IDXGIFactory* factory, IUnknown *pDevice, IUnknown *pWindow, const DXGI_SWAP_CHAIN_DESC1 *pDesc, IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain);
HRESULT WINAPI HookIDXGIFactoryCreateSwapChainForComposition(IDXGIFactory* factory, IUnknown *pDevice, const DXGI_SWAP_CHAIN_DESC1 *pDesc, IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain);
HRESULT WINAPI HookIDXGISwapChainGetBuffer(IDXGISwapChain1 * swapchain, UINT Buffer, REFIID riid, void **ppSurface);
HRESULT WINAPI HookIDXGISwapChainPresent(IDXGISwapChain1* swapchain, UINT SyncInterval, UINT PresentFlags);
HRESULT WINAPI HookIDXGISwapChainPresent1(IDXGISwapChain1* swapchain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS *pPresentParameters);
ULONG WINAPI HookIDXGISwapChainRelease(IDXGISwapChain* swapChain);
