#include <Backends/DX12/RootSignature.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/RootSignatureState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Export/ShaderExportHost.h>

RootRegisterBindingInfo GetBindingInfo(const D3D12_ROOT_SIGNATURE_DESC& source) {
    uint32_t userRegisterSpaceBound = 0;

    // Get the user bound
    for (uint32_t i = 0; i < source.NumParameters; i++) {
        const D3D12_ROOT_PARAMETER& parameter = source.pParameters[i];
        switch (parameter.ParameterType) {
            case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE: {
                for (uint32_t j = 0; j < parameter.DescriptorTable.NumDescriptorRanges; j++) {
                    const D3D12_DESCRIPTOR_RANGE& range = parameter.DescriptorTable.pDescriptorRanges[j];
                    userRegisterSpaceBound = std::max(userRegisterSpaceBound, range.RegisterSpace + 1);
                }
                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS: {
                userRegisterSpaceBound = std::max(userRegisterSpaceBound, parameter.Constants.RegisterSpace + 1);
                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_CBV:
            case D3D12_ROOT_PARAMETER_TYPE_SRV: {
                userRegisterSpaceBound = std::max(userRegisterSpaceBound, parameter.Descriptor.RegisterSpace + 1);
                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_UAV: {
                userRegisterSpaceBound = std::max(userRegisterSpaceBound, parameter.Descriptor.RegisterSpace + 1);
                break;
            }
        }
    }

    // Prepare space
    RootRegisterBindingInfo bindingInfo;
    bindingInfo.space = userRegisterSpaceBound;
    bindingInfo._register = 0;
    return bindingInfo;
}

HRESULT SerializeRootSignature1_0(DeviceState* state, const D3D12_ROOT_SIGNATURE_DESC& source, ID3DBlob** out, RootRegisterBindingInfo* outRoot, ID3DBlob** error) {
    *outRoot = GetBindingInfo(source);

    // Copy parameters
    auto* parameters = ALLOCA_ARRAY(D3D12_ROOT_PARAMETER, source.NumParameters + 1);
    std::memcpy(parameters, source.pParameters, sizeof(D3D12_ROOT_PARAMETER) * source.NumParameters);

    // Shader export range
    D3D12_DESCRIPTOR_RANGE exportRange{};
    exportRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    exportRange.OffsetInDescriptorsFromTableStart = 0;
    exportRange.RegisterSpace = outRoot->space;
    exportRange.BaseShaderRegister = outRoot->_register;
    exportRange.NumDescriptors = 1 + state->exportHost->GetBound();

    // Shader export parameter
    D3D12_ROOT_PARAMETER& exportParameter = parameters[source.NumParameters];
    exportParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    exportParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    exportParameter.DescriptorTable.NumDescriptorRanges = 1;
    exportParameter.DescriptorTable.pDescriptorRanges = &exportRange;

    // Copy signature
    D3D12_ROOT_SIGNATURE_DESC signature = source;
    signature.pParameters = parameters;
    signature.NumParameters = source.NumParameters + 1;

    // Create it
    return D3D12SerializeRootSignature(
        &signature,
        D3D_ROOT_SIGNATURE_VERSION_1,
        out,
        error
    );
}

HRESULT HookID3D12DeviceCreateRootSignature(ID3D12Device *device, UINT nodeMask, const void *blob, SIZE_T length, const IID& riid, void ** pRootSignature) {
    auto table = GetTable(device);

    // Temporary deserializer for re-serialization
    ID3D12VersionedRootSignatureDeserializer* deserializer{nullptr};

    // Immediate deserialization
    HRESULT hr = D3D12CreateVersionedRootSignatureDeserializer(blob, length, IID_PPV_ARGS(&deserializer));
    if (FAILED(hr)) {
        return hr;
    }

    // Unconverted
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* unconverted = deserializer->GetUnconvertedRootSignatureDesc();

#ifndef NDEBUG
    ID3DBlob* error{ nullptr };
#endif // NDEBUG

    // Populated binding info
    RootRegisterBindingInfo bindingInfo;

    // Number of user parameters
    uint32_t userRootCount = 0;

    // Attempt to re-serialize
    ID3DBlob* serialized{nullptr};
    switch (unconverted->Version) {
        default:
            ASSERT(false, "Invalid root signature version");
            return E_INVALIDARG;
        case D3D_ROOT_SIGNATURE_VERSION_1: {
            userRootCount = unconverted->Desc_1_0.NumParameters;

#ifndef NDEBUG
            hr = SerializeRootSignature1_0(table.state, unconverted->Desc_1_0, &serialized, &bindingInfo, &error);
#else // NDEBUG
            hr = SerializeRootSignature1_0(table.state, unconverted->Desc_1_0, &serialized, nullptr);
#endif // NDEBUG
            break;
        }
        case D3D_ROOT_SIGNATURE_VERSION_1_1: {
            ASSERT(false, "Not implemented");
            return E_INVALIDARG;
        }
    }

    // OK?
    if (FAILED(hr)) {
#ifndef NDEBUG
        auto* errorMessage = static_cast<const char*>(error->GetBufferPointer());
        ASSERT(false, errorMessage);
#endif // NDEBUG
        return hr;
    }

    // Cleanup
    deserializer->Release();

    // Object
    ID3D12RootSignature* rootSignature{nullptr};

    // Pass down callchain
    hr = table.bottom->next_CreateRootSignature(table.next, nodeMask, serialized->GetBufferPointer(), serialized->GetBufferSize(), __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&rootSignature));
    if (FAILED(hr)) {
        serialized->Release();
        return hr;
    }

    // Cleanup
    serialized->Release();

    // Create state
    auto* state = new RootSignatureState();
    state->parent = table.state;
    state->rootBindingInfo = bindingInfo;
    state->userRootCount = userRootCount;

    // Create detours
    rootSignature = CreateDetour(Allocators{}, rootSignature, state);

    // Query to external object if requested
    if (pRootSignature) {
        hr = rootSignature->QueryInterface(riid, pRootSignature);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    rootSignature->Release();

    // OK
    return S_OK;
}

ULONG WINAPI HookID3D12RootSignatureRelease(ID3D12RootSignature* signature) {
    auto table = GetTable(signature);

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
