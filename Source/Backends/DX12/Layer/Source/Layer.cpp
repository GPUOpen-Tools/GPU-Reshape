#include <Backends/DX12/Layer.h>

/// Shared validation info
std::optional<D3D12_DEVICE_GPUOPEN_GPU_VALIDATION_INFO> D3D12DeviceGPUOpenGPUValidationInfo;

DX12_C_LINKAGE HRESULT WINAPI D3D12SetDeviceGPUOpenGPUValidationInfo(const D3D12_DEVICE_GPUOPEN_GPU_VALIDATION_INFO* info) {
    if (!info) {
        return S_FALSE;
    }

    D3D12DeviceGPUOpenGPUValidationInfo = *info;
    return S_OK;
}
