#include <Backends/DX12/DescriptorHeap.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/DescriptorHeapState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Export/ShaderExportDescriptorAllocator.h>

HRESULT WINAPI HookID3D12DeviceCreateDescriptorHeap(ID3D12Device *device, const D3D12_DESCRIPTOR_HEAP_DESC *desc, REFIID riid, void **pHeap) {
    auto table = GetTable(device);

    // Create state
    auto* state = new DescriptorHeapState();
    state->parent = table.state;
    state->type = desc->Type;
    state->exhausted = false;

    // Object
    ID3D12DescriptorHeap* heap{nullptr};

    // Heap of interest?
    if (desc->Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
        // Get desired bound
        uint32_t bound = ShaderExportDescriptorAllocator::GetDescriptorBound(table.state->exportHost.GetUnsafe());

        // Copy description
        D3D12_DESCRIPTOR_HEAP_DESC expandedHeap = *desc;
        expandedHeap.NumDescriptors += bound;

        // Pass down callchain
        HRESULT hr = table.bottom->next_CreateDescriptorHeap(table.next, &expandedHeap, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&heap));
        if (SUCCEEDED(hr)) {
            // Create unique allocator
            state->allocator = new (table.state->allocators) ShaderExportDescriptorAllocator(table.next, heap, bound);
        }
    }

    // If exhausted, fall back to user requirements
    if (!heap) {
        state->exhausted = true;

        // Pass down callchain
        HRESULT hr = table.bottom->next_CreateDescriptorHeap(table.next, desc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&heap));
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Create detours
    heap = CreateDetour(Allocators{}, heap, state);

    // Query to external object if requested
    if (pHeap) {
        HRESULT hr = heap->QueryInterface(riid, pHeap);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    heap->Release();

    // OK
    return S_OK;
}

void WINAPI HookID3D12DescriptorHeapGetCPUDescriptorHandleForHeapStart(ID3D12DescriptorHeap *heap, D3D12_CPU_DESCRIPTOR_HANDLE* out) {
    auto table = GetTable(heap);

    D3D12_CPU_DESCRIPTOR_HANDLE reg;
    table.bottom->next_GetCPUDescriptorHandleForHeapStart(table.next, &reg);

    *out = reg;
}

void WINAPI HookID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStart(ID3D12DescriptorHeap *heap, D3D12_GPU_DESCRIPTOR_HANDLE* out) {
    auto table = GetTable(heap);

    D3D12_GPU_DESCRIPTOR_HANDLE reg;
    table.bottom->next_GetGPUDescriptorHandleForHeapStart(table.next, &reg);

    *out = reg;
}

ULONG WINAPI HookID3D12DescriptorHeapRelease(ID3D12DescriptorHeap *heap) {
    auto table = GetTable(heap);

    // Pass down callchain
    LONG users = table.bottom->next_Release(table.next);
    if (users) {
        return users;
    }

    // Cleanup
    delete table.state;

    // OK
    return 0;
}
