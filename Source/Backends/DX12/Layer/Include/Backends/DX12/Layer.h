// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

// System
#include <d3d12.h>

// AMD
#define AGS_GCC 1
#include <AMDAGS/amd_ags.h>

// Std
#include <optional>
#include <atomic>

// Cleanup
#undef OPAQUE
#undef min
#undef max

// Linkage
#ifdef DX12_PRIVATE
#   define DX12_LINKAGE __declspec(dllexport)
#   define DX12_C_LINKAGE extern "C" __declspec(dllexport)
#else // DX12_PRIVATE
#   define DX12_LINKAGE __declspec(dllimport)
#   define DX12_C_LINKAGE extern "C" __declspec(dllimport)
#endif // DX12_PRIVATE

// Forward declarations
class Registry;

/// Function pointer types
using PFN_D3D11_ON_12_CREATE_DEVICE = HRESULT(WINAPI*)(IUnknown *pDevice, UINT Flags, const D3D_FEATURE_LEVEL *pFeatureLevels, UINT FeatureLevels, IUnknown* const *ppCommandQueues, UINT NumQueues, UINT NodeMask, ID3D11Device **ppDevice, ID3D11DeviceContext **ppImmediateContext, D3D_FEATURE_LEVEL *pChosenFeatureLevel);
using PFN_CREATE_DXGI_FACTORY = HRESULT(WINAPI*)(REFIID riid, _COM_Outptr_ void **ppFactory);
using PFN_CREATE_DXGI_FACTORY1 = HRESULT(WINAPI*)(REFIID riid, _COM_Outptr_ void **ppFactory);
using PFN_CREATE_DXGI_FACTORY2 = HRESULT(WINAPI*)(UINT flags, REFIID riid, _COM_Outptr_ void **ppFactory);
using PFN_ENABLE_EXPERIMENTAL_FEATURES = HRESULT(WINAPI*)(UINT NumFeatures, const IID *riid, void *pConfigurationStructs, UINT *pConfigurationStructSizes);
using PFN_D3D12_SET_DEVICE_GPUOPEN_GPU_RESHAPE_INFO = HRESULT (WINAPI *)(const struct D3D12_DEVICE_GPUOPEN_GPU_RESHAPE_INFO* info);
using PFN_D3D12_CREATE_DEVICE_GPUOPEN = HRESULT (WINAPI *)(IUnknown *pAdapter, D3D_FEATURE_LEVEL minimumFeatureLevel, REFIID riid, void **ppDevice, const struct D3D12_DEVICE_GPUOPEN_GPU_RESHAPE_INFO* info);
using PFN_D3D12_SET_FUNCTION_TABLE_GPUOPEN = HRESULT(WINAPI *)(const struct D3D12GPUOpenFunctionTable* table);
using PFN_D3D12_GET_GPUOPEN_BOOTSTRAPPER_INFO = void(WINAPI *)(const struct D3D12GPUOpenBootstrapperInfo* out);

/// Extension pointer types
using PFN_AMD_AGS_CREATE_DEVICE = AGSReturnCode(__stdcall *)(AGSContext* context, const AGSDX12DeviceCreationParams* creationParams, const AGSDX12ExtensionParams* extensionParams, AGSDX12ReturnedParams* returnedParams);
using PFN_AMD_AGS_DESTRIY_DEVICE = AGSReturnCode(__stdcall *)(AGSContext* context, ID3D12Device* device, unsigned int* deviceReferences);
using PFN_AMD_AGS_PUSH_MARKER = AGSReturnCode(__stdcall *)(AGSContext* context, ID3D12GraphicsCommandList* commandList, const char* data);
using PFN_AMD_AGS_POP_MARKER = AGSReturnCode(__stdcall *)(AGSContext* context, ID3D12GraphicsCommandList* commandList);
using PFN_AMD_AGS_SET_MARKER = AGSReturnCode(__stdcall *)(AGSContext* context, ID3D12GraphicsCommandList* commandList, const char* data);

/// Vendor specific device IID
static constexpr GUID kIIDD3D12DeviceVendor = { 0xc443b53a, 0xe4f6, 0x48f5, { 0x98, 0xed, 0xbe, 0x76, 0x8b, 0x47, 0xf, 0x6d } };

/// Optional gpu reshape information
struct D3D12_DEVICE_GPUOPEN_GPU_RESHAPE_INFO {
    /// Shared registry
    Registry* registry;
};

/// Optional function table
struct D3D12GPUOpenFunctionTable {
    PFN_D3D12_CREATE_DEVICE  next_D3D12CreateDeviceOriginal{nullptr};
    PFN_CREATE_DXGI_FACTORY  next_CreateDXGIFactoryOriginal{nullptr};
    PFN_CREATE_DXGI_FACTORY1 next_CreateDXGIFactory1Original{nullptr};
    PFN_CREATE_DXGI_FACTORY2 next_CreateDXGIFactory2Original{nullptr};

    /// Wrappers
    PFN_D3D11_ON_12_CREATE_DEVICE next_D3D11On12CreateDeviceOriginal{nullptr};

    /// Optional
    PFN_ENABLE_EXPERIMENTAL_FEATURES next_EnableExperimentalFeatures{nullptr};

    /// Extensions
    PFN_AMD_AGS_CREATE_DEVICE  next_AMDAGSCreateDevice{nullptr};
    PFN_AMD_AGS_DESTRIY_DEVICE next_AMDAGSDestroyDevice{nullptr};
    PFN_AMD_AGS_PUSH_MARKER    next_AMDAGSPushMarker{nullptr};
    PFN_AMD_AGS_POP_MARKER     next_AMDAGSPopMarker{nullptr};
    PFN_AMD_AGS_SET_MARKER     next_AMDAGSSetMarker{nullptr};
};

/// Process state
struct D3D12GPUOpenProcessState {
    /// Runtime feature set
    bool isExperimentalModeEnabled{false};
    bool isExperimentalShaderModelsEnabled{false};
    bool applicationRequestedExperimentalShadingModels{false};
    bool isDXBCConversionEnabled{false};

    /// Device UID allocator
    std::atomic<uint32_t> deviceUID; 
};

/// Bootstrapper info
struct D3D12GPUOpenBootstrapperInfo {
    uint32_t version;
};

/// Shared validation info
extern std::optional<D3D12_DEVICE_GPUOPEN_GPU_RESHAPE_INFO> D3D12DeviceGPUOpenGPUReshapeInfo;

/// Shared function table
extern D3D12GPUOpenFunctionTable D3D12GPUOpenFunctionTableNext;

/// Shared process info
extern D3D12GPUOpenProcessState D3D12GPUOpenProcessInfo;

/// Prototypes
/// Set the shared validation info
DX12_C_LINKAGE HRESULT WINAPI D3D12SetDeviceGPUOpenGPUReshapeInfo(const D3D12_DEVICE_GPUOPEN_GPU_RESHAPE_INFO* info);

/// Extended device creation
///   ! While cleaner, this has the unintended problem that further hooking will not work, and causes *serious* havok
DX12_C_LINKAGE HRESULT WINAPI D3D12CreateDeviceGPUOpen(
    IUnknown *pAdapter,
    D3D_FEATURE_LEVEL minimumFeatureLevel,
    REFIID riid, void **ppDevice,
    const D3D12_DEVICE_GPUOPEN_GPU_RESHAPE_INFO* info
);

/// Set the internal function table
DX12_C_LINKAGE HRESULT WINAPI D3D12SetFunctionTableGPUOpen(const D3D12GPUOpenFunctionTable* table);
