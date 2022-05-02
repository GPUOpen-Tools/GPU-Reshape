#include <Backends/DX12/Device.h>
#include <Backends/DX12/Resource.h>
#include <Backends/DX12/Fence.h>
#include <Backends/DX12/RootSignature.h>
#include <Backends/DX12/Pipeline.h>
#include <Backends/DX12/CommandList.h>
#include <Backends/DX12/Detour.Gen.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/DeviceState.h>

// Detour
#include <Detour/detours.h>

/// Per-image device creation handle
PFN_D3D12_CREATE_DEVICE D3D12CreateDeviceOriginal{nullptr};

HRESULT WINAPI HookID3D12CreateDevice(
    _In_opt_ IUnknown *pAdapter,
    D3D_FEATURE_LEVEL minimumFeatureLevel,
    _In_ REFIID riid,
    _COM_Outptr_opt_ void **ppDevice) {
    // Object
    ID3D12Device *device{nullptr};

    // Pass down callchain
    HRESULT hr = D3D12CreateDeviceOriginal(pAdapter, minimumFeatureLevel, IID_PPV_ARGS(&device));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new DeviceState();

    // Create detours
    device = CreateDetour(Allocators{}, device, state);

    // Query to external object if requested
    if (ppDevice) {
        hr = device->QueryInterface(riid, ppDevice);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    device->Release();

    // OK
    return S_OK;
}

ULONG HookID3D12DeviceRelease(ID3D12Device* device) {
    auto table = GetTable(device);

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

bool GlobalDeviceDetour::Install() {
    ASSERT(!D3D12CreateDeviceOriginal, "Global device detour re-entry");

    // Attempt to find module
    HMODULE handle = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, L"d3d12.dll", &handle)) {
        return false;
    }

    // Attach against original address
    D3D12CreateDeviceOriginal = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(handle, "D3D12CreateDevice"));
    DetourAttach(&reinterpret_cast<void*&>(D3D12CreateDeviceOriginal), reinterpret_cast<void*>(HookID3D12CreateDevice));

    // OK
    return true;
}

void GlobalDeviceDetour::Uninstall() {
    // Detach from detour
    DetourDetach(&reinterpret_cast<void*&>(D3D12CreateDeviceOriginal), reinterpret_cast<void*>(HookID3D12CreateDevice));
}
