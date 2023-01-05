#pragma once

// Backend
#include <Backends/DX12/DX12.h>

// Forward declarations
class DeviceState;
struct RootRegisterBindingInfo;
struct RootSignaturePhysicalMapping;

/// Serialize a root signature for instrumentation
/// \param state device state
/// \param version signature version
/// \param source signature data
/// \param out serialized blob
/// \param outRoot root bindings
/// \param outMapping root physical mappings
/// \param outError optional, error blob
/// \return result
template<typename T>
HRESULT SerializeRootSignature(DeviceState* state, D3D_ROOT_SIGNATURE_VERSION version, const T& source, ID3DBlob** out, RootRegisterBindingInfo* outRoot, RootSignaturePhysicalMapping** outMapping, ID3DBlob** outError);

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreateRootSignature(ID3D12Device*, UINT, const void*, SIZE_T, const IID&, void**);
HRESULT WINAPI HookID3D12RootSignatureGetDevice(ID3D12RootSignature* _this, REFIID riid, void **ppDevice);
