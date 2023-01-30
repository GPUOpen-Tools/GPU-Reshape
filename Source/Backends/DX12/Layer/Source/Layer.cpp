#include <Backends/DX12/Layer.h>

/// Shared validation info
std::optional<D3D12_DEVICE_GPUOPEN_GPU_VALIDATION_INFO> D3D12DeviceGPUOpenGPUValidationInfo;

/// Shared function table
D3D12GPUOpenFunctionTable D3D12GPUOpenFunctionTableNext;

/// Shared process info
D3D12GPUOpenProcessState D3D12GPUOpenProcessInfo;

DX12_C_LINKAGE HRESULT WINAPI D3D12SetDeviceGPUOpenGPUValidationInfo(const D3D12_DEVICE_GPUOPEN_GPU_VALIDATION_INFO* info) {
    if (!info) {
        return S_FALSE;
    }

    D3D12DeviceGPUOpenGPUValidationInfo = *info;
    return S_OK;
}

DX12_C_LINKAGE HRESULT WINAPI D3D12SetFunctionTableGPUOpen(const D3D12GPUOpenFunctionTable* table) {
    if (!table) {
        return S_FALSE;
    }

    D3D12GPUOpenFunctionTableNext = *table;
    return S_OK;
}
