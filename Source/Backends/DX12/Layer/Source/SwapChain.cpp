#include <Backends/DX12/SwapChain.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/SwapChainState.h>
#include <Backends/DX12/States/ResourceState.h>
#include <Backends/DX12/States/CommandQueueState.h>

static void CreateSwapchainBufferWrappers(SwapChainState* state, uint32_t count) {
    auto deviceTable = GetTable(state->parent);
    
    // Set count
    state->buffers.resize(count);

    // Wrap all resources
    for (uint32_t i = 0; i < count; i++) {
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
        bufferState->parent = state->parent;

        // Add state
        deviceTable.state->states_Resources.Add(bufferState);

        // Create detours
        state->buffers[i] = CreateDetour(state->allocators, bottomBuffer, bufferState);
    }
}

template<typename T, typename U>
static T* CreateSwapChainState(const DXGIFactoryTable& table, ID3D12Device* device, T* swapChain, U* desc) {
    // Create state
    auto* state = new (table.state->allocators, kAllocStateSwapchain) SwapChainState(table.state->allocators);
    state->allocators = table.state->allocators;
    state->parent = device;
    state->object = swapChain;
    state->buffers.resize(desc->BufferCount);

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

        // Fill
        auto table = GetTable(queue);
        out.next = table.next;
        out.device = table.state->parent;
        return out;
    }

    // Per D3D11, the opaque device is a device
    ID3D12Device* device{nullptr};
    if (SUCCEEDED(pDevice->QueryInterface(__uuidof(ID3D12Device*), reinterpret_cast<void**>(&device)))) {
        // Immediately release the handle, parent keeps ownership
        device->Release();

        // Fill
        auto table = GetTable(device);
        out.next = table.next;
        out.device = device;
        return out;
    }

    // Unknown
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

    // Create state
    swapChain = CreateSwapChainState(table, device.device, swapChain, pDesc);

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

    // Create state
    swapChain = CreateSwapChainState(table, device.device, swapChain, pDesc);

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

    // Create state
    swapChain = CreateSwapChainState(table, device.device, swapChain, pDesc);

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

    // Create state
    swapChain = CreateSwapChainState(table, device.device, swapChain, pDesc);

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

    // Cleanup
    table.state->buffers.clear();

    // Pass down callchain
    HRESULT hr = table.bottom->next_ResizeBuffers(table.next, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    if (FAILED(hr)) {
        return hr;
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

    // Cleanup
    table.state->buffers.clear();

    // Pass down callchain
    HRESULT hr = table.bottom->next_ResizeBuffers1(table.next, BufferCount, Width, Height, Format, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
    if (FAILED(hr)) {
        return hr;
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

HRESULT WINAPI HookIDXGISwapChainPresent(IDXGISwapChain1* swapchain, UINT SyncInterval, UINT PresentFlags) {
    auto table = GetTable(swapchain);

    // Pass down callchain
    HRESULT hr = table.bottom->next_Present(table.next, SyncInterval, PresentFlags);
    if (FAILED(hr)) {
        return hr;
    }

    // Add bridge sync
    BridgeDeviceSyncPoint(GetTable(table.state->parent).state);

    // OK
    return S_OK;
}

HRESULT WINAPI HookIDXGISwapChainPresent1(IDXGISwapChain1* swapchain, UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS *pPresentParameters) {
    auto table = GetTable(swapchain);

    // Pass down callchain
    HRESULT hr = table.bottom->next_Present1(table.next, SyncInterval, PresentFlags, pPresentParameters);
    if (FAILED(hr)) {
        return hr;
    }

    // Add bridge sync
    BridgeDeviceSyncPoint(GetTable(table.state->parent).state);

    // OK
    return S_OK;
}

HRESULT HookIDXGISwapChainGetDevice(IDXGISwapChain *_this, const IID &riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevice);
}

SwapChainState::~SwapChainState() {

}
