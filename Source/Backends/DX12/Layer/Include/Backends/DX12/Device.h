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

#pragma once

// Backend
#include <Backends/DX12/DX12.h>
#include <Backends/DX12/Layer.h>

// Forward declarations
struct DeviceState;
struct CommandQueueState;

struct GlobalDeviceDetour {
public:
    bool Install();
    void Uninstall();
};

/// Hooks
DX12_C_LINKAGE HRESULT WINAPI HookD3D12GetInterface(REFCLSID rclsid, REFIID riid, void** ppvDebug);
DX12_C_LINKAGE HRESULT WINAPI HookID3D12CreateDevice(IUnknown *pAdapter, D3D_FEATURE_LEVEL minimumFeatureLevel, REFIID riid, void **ppDevice);
DX12_C_LINKAGE HRESULT WINAPI HookD3D12EnableExperimentalFeatures(UINT NumFeatures, const IID *riid, void *pConfigurationStructs, UINT *pConfigurationStructSizes);
HRESULT WINAPI HookID3D12DeviceCheckFeatureSupport(ID3D12Device* device, D3D12_FEATURE Feature, void *pFeatureSupportData, UINT FeatureSupportDataSize);

/// SDK Hooks
HRESULT WINAPI HookID3D12SDKConfigurationCreateDeviceFactory(ID3D12SDKConfiguration *_this, UINT version, LPCSTR path, IID iid, void **ppvFactory);
HRESULT WINAPI HookID3D12DeviceFactoryEnableExperimentalFeatures(ID3D12DeviceFactory *_this, UINT version, const IID *iid, void *pConfigurationStructs, UINT *pConfigurationStructSizes);
HRESULT WINAPI HookID3D12DeviceFactoryCreateDevice(ID3D12DeviceFactory *_this, IUnknown *unknown, D3D_FEATURE_LEVEL featureLevel, IID iid, void **ppvDevice);

/// Extension hooks
DX12_C_LINKAGE AGSReturnCode HookAMDAGSInitialize(int agsVersion, const AGSConfiguration* config, AGSContext** context, AGSGPUInfo* gpuInfo);
DX12_C_LINKAGE AGSReturnCode HookAMDAGSDeinitialize(AGSContext* context);
DX12_C_LINKAGE AGSReturnCode HookAMDAGSCreateDevice(AGSContext* context, const AGSDX12DeviceCreationParams* creationParams, const AGSDX12ExtensionParams* extensionParams, AGSDX12ReturnedParams* returnedParams);

/// Conditionally enable the experimental shading models for additional feature support
void ConditionallyEnableExperimentalMode();

/// Conditionally create the device factory for SDK overrides
void ConditionallyCreateDeviceFactory();

/// Create a new device
/// \param device target device
/// \param pAdapter user supplied adapter
/// \param minimumFeatureLevel expected minimum feature level
/// \param riid expected target iid
/// \param ppDevice target device
/// \param sdk internal sdk runtime information
/// \param info optional, creation time configuration
/// \return status code
HRESULT WINAPI D3D12CreateDeviceGPUOpen(
    ID3D12Device* device,
    IUnknown *pAdapter,
    D3D_FEATURE_LEVEL minimumFeatureLevel,
    REFIID riid, void **ppDevice,
    const D3D12GPUOpenSDKRuntime& sdk,
    const D3D12_DEVICE_GPUOPEN_GPU_RESHAPE_INFO* info
);

/// Commit all bridge activity on a device
/// \param device device to be committed
/// \param queueState optional, streaming queue to check
void BridgeDeviceSyncPoint(DeviceState *device, CommandQueueState* queueState);
