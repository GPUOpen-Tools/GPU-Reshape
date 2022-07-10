#pragma once

// System
#include <d3d12.h>

// Std
#include <optional>

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

/// Optional gpu validation information
struct D3D12_DEVICE_GPUOPEN_GPU_VALIDATION_INFO {
    /// Shared registry
    Registry* registry;
};

/// Shared validation info
extern std::optional<D3D12_DEVICE_GPUOPEN_GPU_VALIDATION_INFO> D3D12DeviceGPUOpenGPUValidationInfo;

/// Prototypes
using PFN_D3D12_SET_DEVICE_GPUOPEN_GPU_VALIDATION_INFO = HRESULT (WINAPI *)(const D3D12_DEVICE_GPUOPEN_GPU_VALIDATION_INFO* info);
using PFN_D3D12_CREATE_DEVICE_GPUOPEN = HRESULT (WINAPI *)(IUnknown *pAdapter, D3D_FEATURE_LEVEL minimumFeatureLevel, REFIID riid, void **ppDevice, const D3D12_DEVICE_GPUOPEN_GPU_VALIDATION_INFO* info);

/// Set the shared validation info
DX12_C_LINKAGE HRESULT WINAPI D3D12SetDeviceGPUOpenGPUValidationInfo(const D3D12_DEVICE_GPUOPEN_GPU_VALIDATION_INFO* info);

/// Extended device creation
///   ! While cleaner, this has the unintended problem that further hooking will not work, and causes *serious* havok
DX12_C_LINKAGE HRESULT WINAPI D3D12CreateDeviceGPUOpen(
    IUnknown *pAdapter,
    D3D_FEATURE_LEVEL minimumFeatureLevel,
    REFIID riid, void **ppDevice,
    const D3D12_DEVICE_GPUOPEN_GPU_VALIDATION_INFO* info
);
