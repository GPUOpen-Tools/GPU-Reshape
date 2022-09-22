#include <Backends/DX12/RootSignature.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/RootSignatureState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Export/ShaderExportHost.h>

template<typename T>
RootRegisterBindingInfo GetBindingInfo(const T& source) {
    uint32_t userRegisterSpaceBound = 0;

    // Get the user bound
    for (uint32_t i = 0; i < source.NumParameters; i++) {
        const auto& parameter = source.pParameters[i];
        switch (parameter.ParameterType) {
            case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE: {
                for (uint32_t j = 0; j < parameter.DescriptorTable.NumDescriptorRanges; j++) {
                    const auto& range = parameter.DescriptorTable.pDescriptorRanges[j];
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

template<typename T>
HRESULT SerializeRootSignature(DeviceState* state, D3D_ROOT_SIGNATURE_VERSION version, const T& source, ID3DBlob** out, RootRegisterBindingInfo* outRoot, ID3DBlob** error) {
    *outRoot = GetBindingInfo(source);

    // Types
    using Parameter = std::remove_const_t<std::remove_pointer_t<decltype(T::pParameters)>>;
    using DescriptorTable = decltype(Parameter::DescriptorTable);
    using Range = std::remove_const_t<std::remove_pointer_t<decltype(DescriptorTable::pDescriptorRanges)>>;

    // Copy parameters
    auto* parameters = ALLOCA_ARRAY(Parameter, source.NumParameters + 1);
    std::memcpy(parameters, source.pParameters, sizeof(Parameter) * source.NumParameters);

    // Shader export range
    Range exportRange{};
    exportRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    exportRange.OffsetInDescriptorsFromTableStart = 0;
    exportRange.RegisterSpace = outRoot->space;
    exportRange.BaseShaderRegister = outRoot->_register;
    exportRange.NumDescriptors = 1 + state->exportHost->GetBound();

    // Shader export parameter
    Parameter& exportParameter = parameters[source.NumParameters];
    exportParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    exportParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    exportParameter.DescriptorTable.NumDescriptorRanges = 1;
    exportParameter.DescriptorTable.pDescriptorRanges = &exportRange;

    // Versioned creation info
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC versioned;
    versioned.Version = version;

    // Copy signatures
    if constexpr(std::is_same_v<T, D3D12_ROOT_SIGNATURE_DESC1>) {
        versioned.Desc_1_1 = source;
        versioned.Desc_1_1.pParameters = parameters;
        versioned.Desc_1_1.NumParameters = source.NumParameters + 1;
    } else {
        versioned.Desc_1_0 = source;
        versioned.Desc_1_0.pParameters = parameters;
        versioned.Desc_1_0.NumParameters = source.NumParameters + 1;
    }

    // Create it
    return D3D12SerializeVersionedRootSignature(
        &versioned,
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
            hr = SerializeRootSignature(table.state, D3D_ROOT_SIGNATURE_VERSION_1, unconverted->Desc_1_0, &serialized, &bindingInfo, &error);
#else // NDEBUG
            hr = SerializeRootSignature(table.state, D3D_ROOT_SIGNATURE_VERSION_1, unconverted->Desc_1_0, &serialized, &bindingInfo, nullptr);
#endif // NDEBUG
            break;
        }
        case D3D_ROOT_SIGNATURE_VERSION_1_1: {
            userRootCount = unconverted->Desc_1_1.NumParameters;

#ifndef NDEBUG
            hr = SerializeRootSignature(table.state, D3D_ROOT_SIGNATURE_VERSION_1_1, unconverted->Desc_1_1, &serialized, &bindingInfo, &error);
#else // NDEBUG
            hr = SerializeRootSignature(table.state, D3D_ROOT_SIGNATURE_VERSION_1_1, unconverted->Desc_1_1, &serialized, &bindingInfo, nullptr);
#endif // NDEBUG
            break;
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
    state->allocators = table.state->allocators;
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

RootSignatureState::~RootSignatureState() {

}
