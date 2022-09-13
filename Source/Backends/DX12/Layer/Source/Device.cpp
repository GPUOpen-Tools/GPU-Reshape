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
#include <Backends/DX12/Compiler/ShaderCompilerDebug.h>
#include <Backends/DX12/Compiler/DXIL/DXILSigner.h>
#include <Backends/DX12/Compiler/DXBC/DXBCSigner.h>
#include <Backends/DX12/Controllers/InstrumentationController.h>
#include <Backends/DX12/Controllers/MetadataController.h>
#include <Backends/DX12/Export/ShaderExportHost.h>
#include <Backends/DX12/Export/ShaderExportStreamAllocator.h>
#include <Backends/DX12/Export/ShaderExportStreamer.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>
#include <Backends/DX12/Symbolizer/ShaderSGUIDHost.h>
#include <Backends/DX12/Layer.h>

// Backend
#include <Backend/EnvironmentInfo.h>
#include <Backend/IFeatureHost.h>
#include <Backend/IFeature.h>

// Bridge
#include <Bridge/IBridge.h>

// Common
#include <Common/IComponentTemplate.h>

// Detour
#include <Detour/detours.h>

/// Per-image device creation handle
PFN_D3D12_CREATE_DEVICE D3D12CreateDeviceOriginal{nullptr};

static bool PoolAndInstallFeatures(DeviceState* state) {
    // Get the feature host
    auto host = state->registry.Get<IFeatureHost>();
    if (!host) {
        return false;
    }

    // All templates
    std::vector<ComRef<IComponentTemplate>> templates;

    // Pool feature count
    uint32_t featureCount;
    host->Enumerate(&featureCount, nullptr);

    // Pool features
    templates.resize(featureCount);
    host->Enumerate(&featureCount, templates.data());

    // Install features
    for (const ComRef<IComponentTemplate>& _template : templates) {
        // Instantiate feature to this registry
        auto feature = Cast<IFeature>(_template->Instantiate(&state->registry));

        // Try to install feature
        if (!feature->Install()) {
            return false;
        }

        state->features.push_back(feature);
    }

    return true;
}

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

        // Get common components
        state->bridge = state->registry.Get<IBridge>();

        // Install the shader export host
        state->exportHost = state->registry.AddNew<ShaderExportHost>();

        // Install the shader sguid host
        state->sguidHost = state->registry.AddNew<ShaderSGUIDHost>(state);
        ENSURE(state->sguidHost->Install(), "Failed to install shader sguid host");

        // Install all features
        ENSURE(PoolAndInstallFeatures(state), "Failed to install features");

        // Install shader debug
#if SHADER_COMPILER_DEBUG
        auto shaderDebug = state->registry.AddNew<ShaderCompilerDebug>();
#endif

        // Install the dxil signer
        auto dxilSigner = state->registry.AddNew<DXILSigner>();
        ENSURE(dxilSigner->Install(), "Failed to install DXIL signer");

        // Install the dxbc signer
        auto dxbcSigner = state->registry.AddNew<DXBCSigner>();
        ENSURE(dxbcSigner->Install(), "Failed to install DXBC signer");

        // Install the shader compiler
        auto shaderCompiler = state->registry.AddNew<ShaderCompiler>(state);
        ENSURE(shaderCompiler->Install(), "Failed to install shader compiler");

        // Install the pipeline compiler
        auto pipelineCompiler = state->registry.AddNew<PipelineCompiler>(state);
        ENSURE(pipelineCompiler->Install(), "Failed to install pipeline compiler");

        // Install the instrumentation controller
        state->instrumentationController = state->registry.AddNew<InstrumentationController>(state);
        ENSURE(state->instrumentationController->Install(), "Failed to install instrumentation controller");

        // Install the instrumentation controller
        state->metadataController = state->registry.AddNew<MetadataController>(state);
        ENSURE(state->metadataController->Install(), "Failed to install metadata controller");

        // Specifying an adapter is optional
        IDXGIAdapter* dxgiAdapter{nullptr};
        if (pAdapter) {
            // Query underlying adapter
            ENSURE(SUCCEEDED(pAdapter->QueryInterface(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&dxgiAdapter))), "Failed to cast adapter");
        } else {
            // Attempt to create a factory
            IDXGIFactory1* factory;
            ENSURE(SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1) , reinterpret_cast<void**>(&factory))), "Failed to create default factory");

            // Query base adapter
            ENSURE(SUCCEEDED(factory->EnumAdapters(0, &dxgiAdapter)), "Failed to query default adapter");

            // Cleanup
            factory->Release();
        }

        // Install the streamer
        auto deviceAllocator = state->registry.AddNew<DeviceAllocator>();
        ENSURE(deviceAllocator->Install(state->object, dxgiAdapter), "Failed to install device allocator");

        // Install the streamer
        auto streamAllocator = state->registry.AddNew<ShaderExportStreamAllocator>();
        ENSURE(streamAllocator->Install(), "Failed to install shader export stream allocator");

        // Install the streamer
        state->exportStreamer = state->registry.AddNew<ShaderExportStreamer>(state);
        ENSURE(state->exportStreamer->Install(), "Failed to install shader export streamer");
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

void BridgeDeviceSyncPoint(DeviceState *device) {
    // Commit controllers
    device->metadataController->Commit();

    // Commit all logging to bridge
    device->logBuffer.Commit(device->bridge.GetUnsafe());

    // Commit bridge
    device->bridge->Commit();
}
