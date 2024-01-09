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
#include <Backends/DX12/Compiler/DXBC/DXBCConverter.h>
#include <Backends/DX12/Controllers/InstrumentationController.h>
#include <Backends/DX12/Controllers/FeatureController.h>
#include <Backends/DX12/Controllers/MetadataController.h>
#include <Backends/DX12/Controllers/VersioningController.h>
#include <Backends/DX12/Controllers/PDBController.h>
#include <Backends/DX12/Export/ShaderExportHost.h>
#include <Backends/DX12/Export/ShaderExportStreamAllocator.h>
#include <Backends/DX12/Export/ShaderExportStreamer.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>
#include <Backends/DX12/ShaderData/ShaderDataHost.h>
#include <Backends/DX12/Symbolizer/ShaderSGUIDHost.h>
#include <Backends/DX12/ShaderProgram/ShaderProgramHost.h>
#include <Backends/DX12/Scheduler/Scheduler.h>
#include <Backends/DX12/WRL.h>
#include <Backends/DX12/Layer.h>

// Backend
#include <Backend/EnvironmentInfo.h>
#include <Backend/StartupEnvironment.h>
#include <Backend/IFeatureHost.h>
#include <Backend/IFeature.h>

// Bridge
#include <Bridge/IBridge.h>

// Message
#include <Message/IMessageStorage.h>

// Common
#ifndef NDEBUG
#include <Common/Allocator/TrackedAllocator.h>
#endif // NDEBUG
#include <Common/IComponentTemplate.h>
#include <Common/GlobalUID.h>

// Detour
#include <Detour/detours.h>

// Std
#ifndef NDEBUG
#include <iostream>
#include <sstream>
#endif // NDEBUG
#include <fstream>

// Debugging allocator
#ifndef NDEBUG
TrackedAllocator trackedAllocator;
#endif // NDEBUG

/// Known GUIDs
static const GUID D3D12ExperimentalShadingModelGUID = GlobalUID::FromString("{76f5573e-f13a-40f5-b297-81ce9e18933f}").AsPlatformGUID();

static void ApplyStartupEnvironment(DeviceState* state) {
    MessageStream stream;
    
    // Attempt to load
    Backend::StartupEnvironment startupEnvironment;
    startupEnvironment.LoadFromConfig(stream);
    startupEnvironment.LoadFromEnvironment(stream);

    // Empty?
    if (stream.IsEmpty()) {
        return;
    }

    // Commit initial stream
    state->bridge->GetInput()->AddStream(stream);
    state->bridge->Commit();
}

static bool PoolAndInstallFeatures(DeviceState* state) {
    // Get the feature host
    auto host = state->registry.Get<IFeatureHost>();
    if (!host) {
        return false;
    }

    // Pool feature count
    uint32_t featureCount;
    host->Install(&featureCount, nullptr, nullptr);

    // Pool features
    state->features.resize(featureCount);
    return host->Install(&featureCount, state->features.data(), &state->registry);
}

static void CreateEventRemappingTable(DeviceState* state) {
    // All data
    std::vector<ShaderDataInfo> data;

    // Pool feature count
    uint32_t dataCount;
    state->shaderDataHost->Enumerate(&dataCount, nullptr, ShaderDataType::Event);

    // Pool features
    data.resize(dataCount);
    state->shaderDataHost->Enumerate(&dataCount, data.data(), ShaderDataType::Event);

    // Current offset
    uint32_t offset = 0;

    // Populate table
    for (const ShaderDataInfo& info : data) {
        if (info.id >= state->eventRemappingTable.Size()) {
            state->eventRemappingTable.Resize(info.id + 1);
        }

        state->eventRemappingTable[info.id] = offset++;
    }
}

static Backend::EnvironmentDeviceInfo GetEnvironmentDeviceInfo(DeviceState* state) {
    // TODO: Does DX12 have the concept of application names?
    Backend::EnvironmentDeviceInfo info;
    info.applicationName = "Unknown";
    info.apiName = "D3D12";
    info.deviceUID = state->uid;
    info.deviceObjects = static_cast<uint32_t>(state->states_Resources.GetCount());
    return info;
}

HRESULT WINAPI D3D12CreateDeviceGPUOpen(
    ID3D12Device* device,
    IUnknown *pAdapter,
    D3D_FEATURE_LEVEL minimumFeatureLevel,
    REFIID riid, void **ppDevice,
    const D3D12_DEVICE_GPUOPEN_GPU_RESHAPE_INFO* info
) {
    // Set allocators
#if !defined(NDEBUG)
    Allocators allocators = trackedAllocator.GetAllocators();
#else // !defined(NDEBUG)
    Allocators allocators = {};
#endif // !defined(NDEBUG)

    // Create state
    auto *state = new (allocators, kAllocStateDevice) DeviceState(allocators.Tag(kAllocStateDevice));
    state->object = device;
    state->uid = D3D12GPUOpenProcessInfo.deviceUID++;

    // Create detours
    device = CreateDetour(state->allocators, device, state);

    // Query to external object if requested
    if (ppDevice) {
        HRESULT hr = device->QueryInterface(riid, ppDevice);
        if (FAILED(hr)) {
            return hr;
        }

        // Find optional create info
        if (info) {
            // Environment is pre-created at this point
            state->registry.SetParent(info->registry);
        } else {
            // Setup info
            Backend::EnvironmentInfo environmentInfo;
            environmentInfo.device = GetEnvironmentDeviceInfo(state);

            // Initialize the standard environment
            state->environment.Install(environmentInfo);

            // Reparent
            state->registry.SetParent(state->environment.GetRegistry());
        }

        // Set registry allocator
        state->registry.SetAllocators(state->allocators.Tag(kAllocRegistry));

        // Get common components
        state->bridge = state->registry.Get<IBridge>();

        // Install the shader export host
        state->exportHost = state->registry.AddNew<ShaderExportHost>(state->allocators);

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

        // Set stride bounds
        state->cpuHeapTable.SetStrideBound(state->object);
        state->gpuHeapTable.SetStrideBound(state->object);

        // Install the streamer
        state->deviceAllocator = state->registry.AddNew<DeviceAllocator>();
        ENSURE(state->deviceAllocator->Install(state->object, dxgiAdapter), "Failed to install device allocator");

        // Install the shader sguid host
        state->sguidHost = state->registry.AddNew<ShaderSGUIDHost>(state);
        ENSURE(state->sguidHost->Install(), "Failed to install shader sguid host");

        // Install the shader data host
        state->shaderDataHost = state->registry.AddNew<ShaderDataHost>(state);
        ENSURE(state->shaderDataHost->Install(), "Failed to install shader data host");

        // Create the program host
        state->shaderProgramHost = state->registry.AddNew<ShaderProgramHost>(state);
        ENSURE(state->shaderProgramHost->Install(), "Failed to install shader program host");

        // Install the scheduler
        state->scheduler = state->registry.AddNew<Scheduler>(state);
        ENSURE(state->scheduler->Install(), "Failed to install scheduler");

        // Install all features
        ENSURE(PoolAndInstallFeatures(state), "Failed to install features");

        // Create remapping table
        CreateEventRemappingTable(state);

        // Create constant remapping table
        state->constantRemappingTable = state->shaderDataHost->CreateConstantMappingTable();

        // Create the proxies / associations between the backend DX12 commands and the features
        CreateDeviceCommandProxies(state);

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

        // Install the dxbc converter
        auto dxbcConverter = state->registry.AddNew<DXBCConverter>();
        ENSURE(dxbcConverter->Install(), "Failed to install DXBC converter");

        // Install the shader compiler
        auto shaderCompiler = state->registry.AddNew<ShaderCompiler>(state);
        ENSURE(shaderCompiler->Install(), "Failed to install shader compiler");

        // Install the pipeline compiler
        auto pipelineCompiler = state->registry.AddNew<PipelineCompiler>(state);
        ENSURE(pipelineCompiler->Install(), "Failed to install pipeline compiler");

        // Install the instrumentation controller
        state->instrumentationController = state->registry.AddNew<InstrumentationController>(state);
        ENSURE(state->instrumentationController->Install(), "Failed to install instrumentation controller");

        // Install the feature controller
        state->featureController = state->registry.AddNew<FeatureController>(state);
        ENSURE(state->featureController->Install(), "Failed to install feature controller");

        // Install the metadata controller
        state->metadataController = state->registry.AddNew<MetadataController>(state);
        ENSURE(state->metadataController->Install(), "Failed to install metadata controller");

        // Install the versioning controller
        state->versioningController = state->registry.AddNew<VersioningController>(state);
        ENSURE(state->versioningController->Install(), "Failed to install versioning controller");

        // Install the versioning controller
        state->pdbController = state->registry.AddNew<PDBController>(state);
        ENSURE(state->pdbController->Install(), "Failed to install PDB controller");

        // Install all user programs, done after feature creation for data pooling
        ENSURE(state->shaderProgramHost->InstallPrograms(), "Failed to install shader program host programs");

        // Install the streamer
        auto streamAllocator = state->registry.AddNew<ShaderExportStreamAllocator>(state);
        ENSURE(streamAllocator->Install(), "Failed to install shader export stream allocator");

        // Install the streamer
        state->exportStreamer = state->registry.AddNew<ShaderExportStreamer>(state);
        ENSURE(state->exportStreamer->Install(), "Failed to install shader export streamer");

        // Query and apply environment
        ApplyStartupEnvironment(state);

        // Finally, post-install all features for late work
        // This must be done after all dependent states are initialized
        for (const ComRef<IFeature>& feature : state->features) {
            ENSURE(feature->PostInstall(), "Failed to post-install feature");
        }
    }

    // Cleanup
    device->Release();

    // OK
    return S_OK;
}

static bool IsSupportedFeatureLevel(IUnknown* opaqueAdapter, D3D_FEATURE_LEVEL featureLevel) {
    WRLComPtr<IDXGIAdapter> adapter;

    // Existing adapter to pool from?
    if (opaqueAdapter) {
        if (FAILED(opaqueAdapter->QueryInterface(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(adapter.GetAddressOf())))) {
            return false;
        }
    } else {
        // Attempt to create a factory
        WRLComPtr<IDXGIFactory1> factory;
        if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1) , reinterpret_cast<void**>(factory.GetAddressOf())))) {
            return false;
        }

        // Query base adapter
        if (FAILED(factory->EnumAdapters(0, &adapter))) {
            return false;
        }
    }

    // Get description
    DXGI_ADAPTER_DESC adapterDesc;
    if (FAILED(adapter->GetDesc(&adapterDesc))) {
        return false;
    }

    // Skip warp adapters
    if (adapterDesc.VendorId == 0x1414 && adapterDesc.DeviceId == 0x8c) {
        return false;
    }
     
    // Do not attempt to support devices under feature level 11_0
    if (featureLevel < D3D_FEATURE_LEVEL_11_0) {
        return false;
    }

    // OK
    return true;
}

[[maybe_unused]]
static bool IsDevelopmentModeEnabled() {
    // Query data
    DWORD value{0};
    DWORD valueLength = sizeof(value);

    // Query development mode value
    if (DWORD result = RegGetValueW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock",
        L"AllowDevelopmentWithoutDevLicense",
        RRF_RT_DWORD,
        nullptr,
        &value,
        &valueLength
    ); result != ERROR_SUCCESS) {
        return false;
    }

    // OK
    return value != 0;
}

static HRESULT EnableExperimentalFeaturesWithFastTrack(UINT NumFeatures, const IID *riid, void *pConfigurationStructs, UINT *pConfigurationStructSizes) {
    // Keep track
    D3D12GPUOpenProcessInfo.isExperimentalModeEnabled = true;

#if USE_EXPERIMENTAL_SHADING_MODELS
    // Copy previous uuids
    UUID* shaderModels = ALLOCA_ARRAY(UUID, NumFeatures + 1u);
    std::memcpy(shaderModels, riid, sizeof(UUID) * NumFeatures);

    // Request experimental shader models
    shaderModels[NumFeatures] = D3D12ExperimentalShadingModelGUID;

    // Pass down callchain
    HRESULT hr = D3D12GPUOpenFunctionTableNext.next_EnableExperimentalFeatures(NumFeatures + 1u, shaderModels, pConfigurationStructs, pConfigurationStructSizes);
    if (FAILED(hr)) {
        // Logging?
        return hr;
    }
    
    // Mark as enabled
    D3D12GPUOpenProcessInfo.isExperimentalShaderModelsEnabled = true;
#else // USE_EXPERIMENTAL_SHADING_MODELS
    // Pass down callchain
    HRESULT hr = D3D12GPUOpenFunctionTableNext.next_EnableExperimentalFeatures(NumFeatures, riid, pConfigurationStructs, pConfigurationStructSizes);
    if (FAILED(hr)) {
        // Logging?
        return hr;
    }
#endif // USE_EXPERIMENTAL_SHADING_MODELS
    
    // OK!
    return S_OK;
}

static void ConditionallyEnableExperimentalMode() {
    // Only attempt this on development modes
    if (static bool DeveloperModeEnabled = IsDevelopmentModeEnabled(); !DeveloperModeEnabled) {
        return;
    }

    // Only invoke on first run, successive device creations are lost otherwise
    if (D3D12GPUOpenProcessInfo.isExperimentalModeEnabled) {
        return;
    }

    EnableExperimentalFeaturesWithFastTrack(0u, nullptr, nullptr, nullptr);
}

HRESULT WINAPI D3D12CreateDeviceGPUOpen(
    IUnknown *pAdapter,
    D3D_FEATURE_LEVEL minimumFeatureLevel,
    REFIID riid, void **ppDevice,
    const D3D12_DEVICE_GPUOPEN_GPU_RESHAPE_INFO* info
) {
    // Object
    ID3D12Device *device{nullptr};

    // Try to enable for faster instrumentation
    ConditionallyEnableExperimentalMode();

    // Pass down callchain
    HRESULT hr = D3D12GPUOpenFunctionTableNext.next_D3D12CreateDeviceOriginal(pAdapter, minimumFeatureLevel, IID_PPV_ARGS(&device));
    if (FAILED(hr)) {
        return hr;
    }

    // Supported?
    if (!IsSupportedFeatureLevel(pAdapter, minimumFeatureLevel)) {
        return device->QueryInterface(riid, ppDevice);
    }

    // Create wrappers and states
    return D3D12CreateDeviceGPUOpen(
        device,
        pAdapter,
        minimumFeatureLevel,
        riid,
        ppDevice,
        info
    );
}

HRESULT WINAPI HookID3D12CreateDevice(
    _In_opt_ IUnknown *pAdapter,
    D3D_FEATURE_LEVEL minimumFeatureLevel,
    _In_ REFIID riid,
    _COM_Outptr_opt_ void **ppDevice) {
    // Object
    ID3D12Device *device{nullptr};

    // Try to enable for faster instrumentation
    ConditionallyEnableExperimentalMode();

    // Pass down callchain
    HRESULT hr = D3D12GPUOpenFunctionTableNext.next_D3D12CreateDeviceOriginal(pAdapter, minimumFeatureLevel, IID_PPV_ARGS(&device));
    if (FAILED(hr)) {
        return hr;
    }

    // Vendor device?
    if (riid == kIIDD3D12DeviceVendor) {
        return device->QueryInterface(__uuidof(ID3D12Device), ppDevice);
    }

    // Supported?
    if (!IsSupportedFeatureLevel(pAdapter, minimumFeatureLevel)) {
        return device->QueryInterface(riid, ppDevice);
    }

    // Create wrappers and states
    return D3D12CreateDeviceGPUOpen(
        device,
        pAdapter,
        minimumFeatureLevel,
        riid,
        ppDevice,
        D3D12DeviceGPUOpenGPUReshapeInfo ? &*D3D12DeviceGPUOpenGPUReshapeInfo : nullptr
    );
}

HRESULT WINAPI HookD3D12EnableExperimentalFeatures(UINT NumFeatures, const IID *riid, void *pConfigurationStructs, UINT *pConfigurationStructSizes) {
    // Keep track of select guids
    for (uint32_t i = 0; i < NumFeatures; i++) {
        if (riid[i] == D3D12ExperimentalShadingModelGUID) {
            D3D12GPUOpenProcessInfo.applicationRequestedExperimentalShadingModels = true;
        }
    }

    // Enable with fast track
    return EnableExperimentalFeaturesWithFastTrack(NumFeatures, riid, pConfigurationStructs, pConfigurationStructSizes);
}

AGSReturnCode HookAMDAGSCreateDevice(AGSContext* context, const AGSDX12DeviceCreationParams* creationParams, const AGSDX12ExtensionParams* extensionParams, AGSDX12ReturnedParams* returnedParams) {
    // Create with base interface
    AGSDX12DeviceCreationParams params = *creationParams;
    params.iid = __uuidof(ID3D12Device);

    // Try to enable for faster instrumentation
    ConditionallyEnableExperimentalMode();

    // Pass down callchain
    AGSReturnCode code = D3D12GPUOpenFunctionTableNext.next_AMDAGSCreateDevice(context, &params, extensionParams, returnedParams);
    if (code != AGS_SUCCESS) {
        return code;
    }

    // Queried device
    decltype(AGSDX12ReturnedParams::pDevice) device;

    // AGS *may* internally call the hooked device creation, double the wrap double the trouble
    if (DeviceState* wrap{nullptr}; SUCCEEDED(returnedParams->pDevice->QueryInterface(__uuidof(DeviceState), reinterpret_cast<void**>(&wrap)))) {
        // Query wrapped to expected interface
        if (FAILED(returnedParams->pDevice->QueryInterface(creationParams->iid, reinterpret_cast<void**>(&device)))) {
            return AGS_FAILURE;
        }

        // Parent caller implicitly adds a reference to the object, release the fake reference
        returnedParams->pDevice->Release();

        // OK
        returnedParams->pDevice = device;
        return AGS_SUCCESS;
    }

    // Create wrapper, iid dictated by creation parameters
    HRESULT hr = D3D12CreateDeviceGPUOpen(
        reinterpret_cast<ID3D12Device*>(returnedParams->pDevice),
        creationParams->pAdapter,
        creationParams->FeatureLevel,
        creationParams->iid,
        reinterpret_cast<void**>(&device),
        D3D12DeviceGPUOpenGPUReshapeInfo ? &*D3D12DeviceGPUOpenGPUReshapeInfo : nullptr
    );
    if (FAILED(hr)) {
        return AGS_FAILURE;
    }

    // OK
    returnedParams->pDevice = device;
    return AGS_SUCCESS;
}

HRESULT WINAPI HookID3D12DeviceCheckFeatureSupport(ID3D12Device* device, D3D12_FEATURE Feature, void *pFeatureSupportData, UINT FeatureSupportDataSize) {
    auto table = GetTable(device);

    // Special cases
    switch (Feature) {
        default: {
            break;
        }
        case D3D12_FEATURE_SHADER_MODEL: {
            auto* typedData = static_cast<D3D12_FEATURE_DATA_SHADER_MODEL*>(pFeatureSupportData);

            // If the user has *not* requested experimental shading models, do not report anything above 6.7
            // Internal fast tracking implicitly enables 6.7, which can cause undesired behaviour in some applications
            if (!D3D12GPUOpenProcessInfo.applicationRequestedExperimentalShadingModels && typedData->HighestShaderModel > D3D_SHADER_MODEL_6_7) {
                return E_INVALIDARG;
            }

            // All ok
            break;
        }
    }

    // Pass down call chain
    return table.next->CheckFeatureSupport(Feature, pFeatureSupportData, FeatureSupportDataSize);
}

DeviceState::~DeviceState() {
    // May not be created
    if (!exportStreamer) {
        return;
    }

    // Wait for all pending instrumentation
    instrumentationController->WaitForCompletion();

    // Process all remaining work
    exportStreamer->Process();

    // Wait for all pending submissions
    scheduler->WaitForPending();

    // Manual uninstalls
    versioningController->Uninstall();
    metadataController->Uninstall();
    instrumentationController->Uninstall();

    // Release all features
    features.clear();
}

bool GlobalDeviceDetour::Install() {
    ASSERT(!D3D12GPUOpenFunctionTableNext.next_D3D12CreateDeviceOriginal, "Global device detour re-entry");

    // Attempt to find module
    HMODULE handle = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, L"d3d12.dll", &handle)) {
        return false;
    }

    // Attach against original address
    D3D12GPUOpenFunctionTableNext.next_D3D12CreateDeviceOriginal = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(handle, "D3D12CreateDevice"));
    DetourAttach(&reinterpret_cast<void*&>(D3D12GPUOpenFunctionTableNext.next_D3D12CreateDeviceOriginal), reinterpret_cast<void*>(HookID3D12CreateDevice));

    // Attach against original address
    D3D12GPUOpenFunctionTableNext.next_EnableExperimentalFeatures = reinterpret_cast<PFN_ENABLE_EXPERIMENTAL_FEATURES>(GetProcAddress(handle, "D3D12EnableExperimentalFeatures"));
    DetourAttach(&reinterpret_cast<void*&>(D3D12GPUOpenFunctionTableNext.next_EnableExperimentalFeatures), reinterpret_cast<void*>(HookD3D12EnableExperimentalFeatures));

    // OK
    return true;
}

void GlobalDeviceDetour::Uninstall() {
    // Detach from detour
    DetourDetach(&reinterpret_cast<void*&>(D3D12GPUOpenFunctionTableNext.next_D3D12CreateDeviceOriginal), reinterpret_cast<void*>(HookID3D12CreateDevice));
    DetourDetach(&reinterpret_cast<void*&>(D3D12GPUOpenFunctionTableNext.next_EnableExperimentalFeatures), reinterpret_cast<void*>(HookD3D12EnableExperimentalFeatures));
}

void BridgeDeviceSyncPoint(DeviceState *device) {
    // Commit all logging to bridge
    device->logBuffer.Commit(device->bridge.GetUnsafe());
    
    // Commit controllers
    device->featureController->Commit();
    device->instrumentationController->Commit();
    device->metadataController->Commit();
    device->versioningController->Commit();

    // Commit bridge
    device->bridge->Commit();

    // Sync the scheduler
    device->scheduler->SyncPoint();

    // Update the environment?
    if (device->environmentUpdateAction.Step()) {
        device->environment.Update(GetEnvironmentDeviceInfo(device));
    }

    // Debugging helper
#ifndef NDEBUG
    if (false) {
        // Format
        std::stringstream stream;
        trackedAllocator.Dump(stream);

        // Dump to console
        OutputDebugStringA(stream.str().c_str());
    }
#endif // NDEBUG
}
