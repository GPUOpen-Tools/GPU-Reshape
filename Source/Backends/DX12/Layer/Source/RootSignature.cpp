#include <Backends/DX12/RootSignature.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/RootSignature.h>
#include <Backends/DX12/States/DeviceState.h>

HRESULT HookID3D12DeviceCreateRootSignature(ID3D12Device *device, UINT nodeMask, const void *blob, SIZE_T length, const IID& riid, void ** pRootSignature) {
    auto table = GetTable(device);

    // Object
    ID3D12RootSignature* rootSignature{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateRootSignature(table.next, nodeMask, blob, length, __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&rootSignature));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto* state = new RootSignatureState();
    state->parent = table.state;

    // Create detours
    CreateDetour(Allocators{}, rootSignature, state);

    // Query to external object if requested
    if (pRootSignature) {
        hr = rootSignature->QueryInterface(riid, pRootSignature);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    rootSignature->Release();

    // OK
    return S_OK;
}

ULONG WINAPI HookID3D12RootSignatureRelease(ID3D12RootSignature* signature) {
    auto table = GetTable(signature);

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
