#include <Backends/DX12/Device.h>
#include <Backends/DX12/Resource.h>
#include <Backends/DX12/Fence.h>
#include <Backends/DX12/RootSignature.h>
#include <Backends/DX12/Pipeline.h>
#include <Backends/DX12/CommandList.h>
#include <Backends/DX12/Detour.Gen.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Compiler/ShaderCompiler.h>
#include <Backends/DX12/Compiler/PipelineCompiler.h>
#include <Backends/DX12/Controllers/InstrumentationController.h>
#include <Backends/DX12/Export/ShaderExportHost.h>
#include <Backends/DX12/Layer.h>

// Backend
#include <Backend/EnvironmentInfo.h>

// Detour
#include <Detour/detours.h>

/// Per-image device creation handle
PFN_D3D12_CREATE_DEVICE D3D12CreateDeviceOriginal{nullptr};

HRESULT WINAPI D3D12CreateDeviceGPUOpen(
    IUnknown *pAdapter,
    D3D_FEATURE_LEVEL minimumFeatureLevel,
    REFIID riid, void **ppDevice,
    const D3D12_DEVICE_GPUOPEN_GPU_VALIDATION_INFO* info
) {
    // Object
    ID3D12Device *device{nullptr};

    // Pass down callchain
    HRESULT hr = D3D12CreateDeviceOriginal(pAdapter, minimumFeatureLevel, IID_PPV_ARGS(&device));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new DeviceState();
    state->object = device;

    // Create detours
    device = CreateDetour(Allocators{}, device, state);

    // Query to external object if requested
    if (ppDevice) {
        hr = device->QueryInterface(riid, ppDevice);
        if (FAILED(hr)) {
            return hr;
        }

        // Find optional create info
        if (info) {
            // Environment is pre-created at this point
            state->registry.SetParent(info->registry);
        } else {
            // Setup info
            // TODO: Does DX12 have the concept of application names?
            Backend::EnvironmentInfo environmentInfo;
            environmentInfo.applicationName = "Unknown";

            // Initialize the standard environment
            state->environment.Install(environmentInfo);

            // Reparent
            state->registry.SetParent(state->environment.GetRegistry());
        }

        // Install the shader export host
        state->registry.AddNew<ShaderExportHost>();

        // Install the shader compiler
        auto shaderCompiler = state->registry.AddNew<ShaderCompiler>(state);
        ENSURE(shaderCompiler->Install(), "Failed to install shader compiler");

        // Install the pipeline compiler
        auto pipelineCompiler = state->registry.AddNew<PipelineCompiler>(state);
        ENSURE(pipelineCompiler->Install(), "Failed to install pipeline compiler");

        // Install the instrumentation controller
        state->instrumentationController = state->registry.New<InstrumentationController>(state);
        ENSURE(state->instrumentationController->Install(), "Failed to install instrumentation controller");
    }

    // Cleanup
    device->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12CreateDevice(
    _In_opt_ IUnknown *pAdapter,
    D3D_FEATURE_LEVEL minimumFeatureLevel,
    _In_ REFIID riid,
    _COM_Outptr_opt_ void **ppDevice) {
    return D3D12CreateDeviceGPUOpen(pAdapter, minimumFeatureLevel, riid, ppDevice, D3D12DeviceGPUOpenGPUValidationInfo ? &*D3D12DeviceGPUOpenGPUValidationInfo : nullptr);
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
