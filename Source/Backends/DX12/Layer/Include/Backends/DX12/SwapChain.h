// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
#include <Backends/DX12/DX12.h>

/// Hooks
HRESULT WINAPI HookIDXGIFactoryCreateSwapChain(IDXGIFactory* factory, IUnknown *pDevice, DXGI_SWAP_CHAIN_DESC *pDesc, IDXGISwapChain **ppSwapChain);
HRESULT WINAPI HookIDXGIFactoryCreateSwapChainForHwnd(IDXGIFactory* factory, IUnknown *pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1 *pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc, IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain);
HRESULT WINAPI HookIDXGIFactoryCreateSwapChainForCoreWindow(IDXGIFactory* factory, IUnknown *pDevice, IUnknown *pWindow, const DXGI_SWAP_CHAIN_DESC1 *pDesc, IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain);
HRESULT WINAPI HookIDXGIFactoryCreateSwapChainForComposition(IDXGIFactory* factory, IUnknown *pDevice, const DXGI_SWAP_CHAIN_DESC1 *pDesc, IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain);
HRESULT WINAPI HookIDXGISwapChainGetBuffer(IDXGISwapChain1 * swapchain, UINT Buffer, REFIID riid, void **ppSurface);
HRESULT WINAPI HookIDXGISwapChainResizeBuffers(IDXGISwapChain1* _this, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
HRESULT WINAPI HookIDXGISwapChainPresent(IDXGISwapChain1* swapchain, UINT SyncInterval, UINT PresentFlags);
HRESULT WINAPI HookIDXGISwapChainPresent1(IDXGISwapChain1* swapchain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS *pPresentParameters);
HRESULT WINAPI HookIDXGISwapChainResizeBuffers1(IDXGISwapChain1* swapchain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT Format, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue);
HRESULT WINAPI HookIDXGISwapChainGetDevice(IDXGISwapChain* swapchain, REFIID riid, void **ppDevice);
HRESULT WINAPI HookIDXGISwapChainGetParent(IDXGISwapChain* swapchain, REFIID riid, void **ppParent);
