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

#include <Backends/DX12/SwapChain.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/SwapChainState.h>
#include <Backends/DX12/States/ResourceState.h>
#include <Backends/DX12/States/CommandQueueState.h>

// Bridge
#include <Bridge/IBridge.h>

// Message
#include <Message/OrderedMessageStorage.h>

// Schemas
#include <Schemas/Diagnostic.h>

static void CreateSwapchainBufferWrappers(SwapChainState* state, uint32_t count) {
    auto deviceTable = GetTable(state->device);
    
    // Set count
    state->buffers.resize(count);

    // Wrap all resources
    for (uint32_t i = 0; i < count; i++) {
        // Release previous buffer
        if (state->buffers[i]) {
            state->buffers[i]->Release();
        }
        
        // Get buffer (increments lifetime by one)
        ID3D12Resource* bottomBuffer{nullptr};
        if (FAILED(state->object->GetBuffer(i, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&bottomBuffer)))) {
            state->buffers[i] = nullptr;
        }

        // Create state
        auto* bufferState = new (state->allocators, kAllocStateResource) ResourceState();
        bufferState->allocators = state->allocators;
        bufferState->object = bottomBuffer;
        bufferState->desc = bottomBuffer->GetDesc();
        bufferState->parent = state->device;

        // Add user
        state->device->AddRef();

        // Add state
        deviceTable.state->states_Resources.Add(bufferState);

        // Create detours
        state->buffers[i] = CreateDetour(state->allocators, bottomBuffer, bufferState);
    }
}

template<typename T, typename U>
static T* CreateSwapChainState(const DXGIFactoryTable& table, IDXGIFactory* factory, ID3D12Device* device, T* swapChain, U* desc) {
    // Create state
    auto* state = new (table.state->allocators, kAllocStateSwapchain) SwapChainState(table.state->allocators);
    state->allocators = table.state->allocators;
    state->parent = factory;
    state->device = device;
    state->object = swapChain;
    state->buffers.resize(desc->BufferCount);

    // Keep reference to parent
    factory->AddRef();

    // Create wrappers
    CreateSwapchainBufferWrappers(state, desc->BufferCount);

    // Create detours
    return static_cast<T*>(CreateDetour(state->allocators, swapChain, state));
}

struct OpaqueDeviceInfo {
    IUnknown* next{nullptr};
    ID3D12Device* device{nullptr};
};

static OpaqueDeviceInfo QueryDeviceFromOpaque(IUnknown* pDevice) {
    OpaqueDeviceInfo out{};

    // Per D3D12, the opaque device is a command queue
    ID3D12CommandQueue* queue{nullptr};
    if (SUCCEEDED(pDevice->QueryInterface(__uuidof(ID3D12CommandQueue*), reinterpret_cast<void**>(&queue)))) {
        // Immediately release the handle, parent keeps ownership
        queue->Release();

        // Fill if wrapped
        if (IsWrapped(queue)) {
            auto table = GetTable(queue);
            out.next = table.next;
            out.device = table.state->parent;
            return out;
        }
    }

    // Per D3D11, the opaque device is a device
    ID3D12Device* device{nullptr};
    if (SUCCEEDED(pDevice->QueryInterface(__uuidof(ID3D12Device*), reinterpret_cast<void**>(&device)))) {
        // Immediately release the handle, parent keeps ownership
        device->Release();

        // Fill if wrapped
        if (IsWrapped(queue)) {
            auto table = GetTable(device);
            out.next = table.next;
            out.device = device;
            return out;
        }
    }

    // Unknown
    out.next = pDevice;
    return out;
}

HRESULT WINAPI HookIDXGIFactoryCreateSwapChain(IDXGIFactory* factory, IUnknown *pDevice, DXGI_SWAP_CHAIN_DESC *pDesc, IDXGISwapChain **ppSwapChain) {
    auto table = GetTable(factory);

    // Get device
    OpaqueDeviceInfo device = QueryDeviceFromOpaque(pDevice);

    // Object
    IDXGISwapChain* swapChain{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateSwapChain(table.next, device.next, pDesc, &swapChain);
    if (FAILED(hr)) {
        return hr;
    }

    // Create state for D3D12 enabled devices
    if (device.device) {
        swapChain = CreateSwapChainState(table, factory, device.device, swapChain, pDesc);
    }

    // Query to external object if requested
    if (ppSwapChain) {
        *ppSwapChain = swapChain;
    } else {
        swapChain->Release();
    }

    // OK
    return S_OK;
}

HRESULT WINAPI HookIDXGIFactoryCreateSwapChainForHwnd(IDXGIFactory* factory, IUnknown *pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1 *pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc, IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain) {
    auto table = GetTable(factory);

    // Get device
    OpaqueDeviceInfo device = QueryDeviceFromOpaque(pDevice);

    // Object
    IDXGISwapChain1* swapChain{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateSwapChainForHwnd(table.next, device.next, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, &swapChain);
    if (FAILED(hr)) {
        return hr;
    }

    // Create state for D3D12 enabled devices
    if (device.device) {
        swapChain = CreateSwapChainState(table, factory, device.device, swapChain, pDesc);
    }

    // Query to external object if requested
    if (ppSwapChain) {
        *ppSwapChain = swapChain;
    } else {
        swapChain->Release();
    }

    // OK
    return S_OK;
}

HRESULT WINAPI HookIDXGIFactoryCreateSwapChainForCoreWindow(IDXGIFactory* factory, IUnknown *pDevice, IUnknown *pWindow, const DXGI_SWAP_CHAIN_DESC1 *pDesc, IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain) {
    auto table = GetTable(factory);

    // Get device
    OpaqueDeviceInfo device = QueryDeviceFromOpaque(pDevice);

    // Object
    IDXGISwapChain1* swapChain{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateSwapChainForCoreWindow(table.next, device.next, pWindow, pDesc, pRestrictToOutput, &swapChain);
    if (FAILED(hr)) {
        return hr;
    }

    // Create state for D3D12 enabled devices
    if (device.device) {
        swapChain = CreateSwapChainState(table, factory, device.device, swapChain, pDesc);
    }

    // Query to external object if requested
    if (ppSwapChain) {
        *ppSwapChain = swapChain;
    } else {
        swapChain->Release();
    }

    // OK
    return S_OK;
}

HRESULT WINAPI HookIDXGIFactoryCreateSwapChainForComposition(IDXGIFactory* factory, IUnknown *pDevice, const DXGI_SWAP_CHAIN_DESC1 *pDesc, IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain) {
    auto table = GetTable(factory);

    // Get device
    OpaqueDeviceInfo device = QueryDeviceFromOpaque(pDevice);

    // Object
    IDXGISwapChain1* swapChain{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateSwapChainForComposition(table.next, device.next, pDesc, pRestrictToOutput, &swapChain);
    if (FAILED(hr)) {
        return hr;
    }

    // Create state for D3D12 enabled devices
    if (device.device) {
        swapChain = CreateSwapChainState(table, factory, device.device, swapChain, pDesc);
    }

    // Query to external object if requested
    if (ppSwapChain) {
        *ppSwapChain = swapChain;
    } else {
        swapChain->Release();
    }

    // OK
    return S_OK;
}

HRESULT WINAPI HookIDXGISwapChainResizeBuffers(IDXGISwapChain1* swapchain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    auto table = GetTable(swapchain);

    // Release wrapped objects
    for (ID3D12Resource* buffer : table.state->buffers) {
        buffer->Release();
    }

    // Pass down callchain
    HRESULT hr = table.bottom->next_ResizeBuffers(table.next, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    if (FAILED(hr)) {
        return hr;
    }

    // If zero, the number of buffers is preserved
    if (!BufferCount) {
        BufferCount = static_cast<uint32_t>(table.state->buffers.size());
    }

    // Recreate wrappers
    CreateSwapchainBufferWrappers(table.state, BufferCount);

    // OK
    return S_OK;
}

HRESULT WINAPI HookIDXGISwapChainResizeBuffers1(IDXGISwapChain1* swapchain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT Format, UINT SwapChainFlags, const UINT* pCreationNodeMask, IUnknown* const* ppPresentQueue) {
    auto table = GetTable(swapchain);

    // Release wrapped objects
    for (ID3D12Resource* buffer : table.state->buffers) {
        buffer->Release();
    }

    // Pass down callchain
    HRESULT hr = table.bottom->next_ResizeBuffers1(table.next, BufferCount, Width, Height, Format, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
    if (FAILED(hr)) {
        return hr;
    }

    // If zero, the number of buffers is preserved
    if (!BufferCount) {
        BufferCount = static_cast<uint32_t>(table.state->buffers.size());
    }

    // Recreate wrappers
    CreateSwapchainBufferWrappers(table.state, BufferCount);

    // OK
    return S_OK;
}

HRESULT WINAPI HookIDXGISwapChainGetBuffer(IDXGISwapChain1 * swapchain, UINT Buffer, REFIID riid, void **ppSurface) {
    auto table = GetTable(swapchain);

    // Bounds check
    if (Buffer > table.state->buffers.size()) {
        return E_FAIL;
    }

    // Validation
#ifndef NDEBUG
    // Get buffer
    ID3D12Resource* bottomBuffer{nullptr};
    if (SUCCEEDED(table.bottom->next_GetBuffer(table.next, Buffer, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&bottomBuffer)))) {
        ASSERT(bottomBuffer == Next(table.state->buffers[Buffer]), "Invalid swapchain buffer");
        bottomBuffer->Release();
    }
#endif // NDEBUG

    // Query to external
    return table.state->buffers[Buffer]->QueryInterface(riid, ppSurface);
}

void HandlePresent(DeviceState* device, SwapChainState* swapchain) {
    // Current time
    std::chrono::time_point<std::chrono::high_resolution_clock> presentTime = std::chrono::high_resolution_clock::now();

    // Setup stream
    MessageStream stream;
    MessageStreamView view(stream);

    // Determine elapsed
    float elapsedMS = std::chrono::duration_cast<std::chrono::nanoseconds>(presentTime - swapchain->lastPresentTime).count() / 1e6f;

    // Add message
    auto* diagnostic = view.Add<PresentDiagnosticMessage>();
    diagnostic->intervalMS = elapsedMS;

    // Set new present time
    swapchain->lastPresentTime = presentTime;

    // Commit stream
    device->bridge->GetOutput()->AddStream(stream);
}

HRESULT WINAPI HookIDXGISwapChainPresent(IDXGISwapChain1* swapchain, UINT SyncInterval, UINT PresentFlags) {
    auto table = GetTable(swapchain);

    // Get device
    auto deviceTable = GetTable(table.state->device);

    // Pass down callchain
    HRESULT hr = table.bottom->next_Present(table.next, SyncInterval, PresentFlags);
    if (FAILED(hr)) {
        return hr;
    }

    // Handle it
    HandlePresent(deviceTable.state, table.state);

    // OK
    return S_OK;
}

HRESULT WINAPI HookIDXGISwapChainPresent1(IDXGISwapChain1* swapchain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS *pPresentParameters) {
    auto table = GetTable(swapchain);

    // Get device
    auto deviceTable = GetTable(table.state->device);

    // Pass down callchain
    HRESULT hr = table.bottom->next_Present1(table.next, SyncInterval, PresentFlags, pPresentParameters);
    if (FAILED(hr)) {
        return hr;
    }

    // Handle it
    HandlePresent(deviceTable.state, table.state);

    // OK
    return S_OK;
}

HRESULT HookIDXGISwapChainGetDevice(IDXGISwapChain *_this, const IID &riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->device->QueryInterface(riid, ppDevice);
}

HRESULT WINAPI HookIDXGISwapChainGetParent(IDXGISwapChain* _this, REFIID riid, void **ppParent) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppParent);
}

SwapChainState::~SwapChainState() {
    for (ID3D12Resource *buffer : buffers) {
        buffer->Release();
    }

    // Release ref
    parent->Release();
}
