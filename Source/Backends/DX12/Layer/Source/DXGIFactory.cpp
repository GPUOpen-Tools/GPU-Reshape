#include <Backends/DX12/DXGIFactory.h>
#include <Backends/DX12/Detour.Gen.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/DXGIFactoryState.h>

// Detour
#include <Detour/detours.h>

/// Per-image device creation handle
PFN_CREATE_DXGI_FACTORY  CreateDXGIFactoryOriginal{nullptr};
PFN_CREATE_DXGI_FACTORY1 CreateDXGIFactory1Original{nullptr};
PFN_CREATE_DXGI_FACTORY2 CreateDXGIFactory2Original{nullptr};

HRESULT WINAPI HookCreateDXGIFactory(REFIID riid, _COM_Outptr_ void **ppFactory) {
    // Object
    IDXGIFactory *factory{nullptr};

    // Pass down callchain
    HRESULT hr = CreateDXGIFactoryOriginal(IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new DXGIFactoryState();

    // Create detours
    factory = CreateDetour(Allocators{}, factory, state);

    // Query to external object if requested
    if (ppFactory) {
        hr = factory->QueryInterface(riid, ppFactory);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    factory->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookCreateDXGIFactory1(REFIID riid, _COM_Outptr_ void **ppFactory) {
    // Object
    IDXGIFactory *factory{nullptr};

    // Pass down callchain
    HRESULT hr = CreateDXGIFactory1Original(IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new DXGIFactoryState();

    // Create detours
    factory = CreateDetour(Allocators{}, factory, state);

    // Query to external object if requested
    if (ppFactory) {
        hr = factory->QueryInterface(riid, ppFactory);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    factory->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookCreateDXGIFactory2(UINT flags, REFIID riid, _COM_Outptr_ void **ppFactory) {
    // Object
    IDXGIFactory *factory{nullptr};

    // Pass down callchain
    HRESULT hr = CreateDXGIFactory2Original(flags, IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new DXGIFactoryState();

    // Create detours
    factory = CreateDetour(Allocators{}, factory, state);

    // Query to external object if requested
    if (ppFactory) {
        hr = factory->QueryInterface(riid, ppFactory);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    factory->Release();

    // OK
    return S_OK;
}

ULONG HookIDXGIFactoryRelease(IDXGIFactory* factory) {
    auto table = GetTable(factory);

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

bool GlobalDXGIFactoryDetour::Install() {
    ASSERT(!CreateDXGIFactoryOriginal, "Global dxgi factory detour re-entry");

    // Attempt to find module
    HMODULE handle = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, L"dxgi.dll", &handle)) {
        return false;
    }

    // Attach against original address
    CreateDXGIFactoryOriginal = reinterpret_cast<PFN_CREATE_DXGI_FACTORY>(GetProcAddress(handle, "CreateDXGIFactory"));
    DetourAttach(&reinterpret_cast<void*&>(CreateDXGIFactoryOriginal), reinterpret_cast<void*>(HookCreateDXGIFactory));

    // Attach against original address
    CreateDXGIFactory1Original = reinterpret_cast<PFN_CREATE_DXGI_FACTORY1>(GetProcAddress(handle, "CreateDXGIFactory1"));
    if (CreateDXGIFactory1Original) {
        DetourAttach(&reinterpret_cast<void*&>(CreateDXGIFactory1Original), reinterpret_cast<void*>(HookCreateDXGIFactory1));
    }

    // Attach against original address
    CreateDXGIFactory2Original = reinterpret_cast<PFN_CREATE_DXGI_FACTORY2>(GetProcAddress(handle, "CreateDXGIFactory2"));
    if (CreateDXGIFactory2Original) {
        DetourAttach(&reinterpret_cast<void *&>(CreateDXGIFactory2Original), reinterpret_cast<void *>(HookCreateDXGIFactory2));
    }

    // OK
    return true;
}

void GlobalDXGIFactoryDetour::Uninstall() {
    // Detach from detour
    DetourDetach(&reinterpret_cast<void*&>(CreateDXGIFactoryOriginal), reinterpret_cast<void*>(HookCreateDXGIFactory));
    if (CreateDXGIFactory1Original) {
        DetourDetach(&reinterpret_cast<void*&>(CreateDXGIFactory1Original), reinterpret_cast<void*>(HookCreateDXGIFactory1));
    }
    if (CreateDXGIFactory2Original) {
        DetourDetach(&reinterpret_cast<void*&>(CreateDXGIFactory2Original), reinterpret_cast<void*>(HookCreateDXGIFactory2));
    }
}
