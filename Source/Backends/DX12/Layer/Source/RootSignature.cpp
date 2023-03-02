#include <Backends/DX12/RootSignature.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/RootSignatureState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/States/RootSignaturePhysicalMapping.h>
#include <Backends/DX12/Export/ShaderExportHost.h>
#include <Backends/DX12/ShaderData/ShaderDataHost.h>

// Common
#include <Common/Hash.h>

template<typename T>
RootRegisterBindingInfo GetBindingInfo(DeviceState* state, const T& source) {
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

    // Current register offset
    uint32_t registerOffset = 0;

    // Set base register for shader exports
    bindingInfo.shaderExportBaseRegister = registerOffset;
    bindingInfo.shaderExportCount = static_cast<uint32_t>(state->features.size()) + 1u;
    registerOffset += bindingInfo.shaderExportCount;

    // Set base register for prmt data
    bindingInfo.prmtBaseRegister = registerOffset;
    registerOffset += 1u;

    // Set base register for descriptor constants
    bindingInfo.descriptorConstantBaseRegister = registerOffset;
    registerOffset += 1u;

    // Set base register for event constants
    bindingInfo.eventConstantBaseRegister = registerOffset;
    registerOffset += 1u;

    // Get number of resources
    uint32_t resourceCount{0};
    state->shaderDataHost->Enumerate(&resourceCount, nullptr, ShaderDataType::DescriptorMask);

    // Set base register for shader exports
    bindingInfo.shaderResourceBaseRegister = registerOffset;
    bindingInfo.shaderResourceCount = std::max(1u, resourceCount);
    registerOffset += bindingInfo.shaderResourceCount;

    return bindingInfo;
}

RootSignatureUserMapping& GetRootMapping(RootSignaturePhysicalMapping* mapping, RootSignatureUserClassType type, uint32_t space, uint32_t offset) {
    RootSignatureUserClass& _class = mapping->spaces[static_cast<uint32_t>(type)];

    if (_class.spaces.size() <= space) {
        _class.spaces.resize(space + 1);
    }

    RootSignatureUserSpace& userSpace = _class.spaces[space];

    if (userSpace.mappings.Size() <= offset) {
        userSpace.mappings.Resize(offset + 1);
    }

    return userSpace.mappings[offset];
}

template<typename T>
static RootSignaturePhysicalMapping* CreateRootPhysicalMappings(DeviceState* state, const T* parameters, uint32_t parameterCount) {
    auto* mapping = new (state->allocators, kAllocState) RootSignaturePhysicalMapping(state->allocators);

    // TODO: Could do a pre-pass

    // Create hash and mappings
    for (uint32_t i = 0; i < parameterCount; i++) {
        const T& parameter = parameters[i];

        // Hash common data
        CombineHash(mapping->signatureHash, parameter.ShaderVisibility);

        // Hash parameter data
        switch (parameter.ParameterType) {
            case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE: {
                // Add to hash
                CombineHash(mapping->signatureHash, parameter.DescriptorTable.NumDescriptorRanges);
                CombineHash(mapping->signatureHash, BufferCRC64(parameter.DescriptorTable.pDescriptorRanges, sizeof(D3D12_DESCRIPTOR_RANGE1) * parameter.DescriptorTable.NumDescriptorRanges));

                // Current descriptor offset
                uint32_t descriptorOffset = 0;

                // Handle all ranges
                for (uint32_t rangeIdx = 0; rangeIdx < parameter.DescriptorTable.NumDescriptorRanges; rangeIdx++) {
                    const auto& range = parameter.DescriptorTable.pDescriptorRanges[rangeIdx];

                    // To class type
                    RootSignatureUserClassType classType;
                    switch (range.RangeType) {
                        case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                            classType = RootSignatureUserClassType::SRV;
                            break;
                        case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                            classType = RootSignatureUserClassType::UAV;
                            break;
                        case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
                            classType = RootSignatureUserClassType::CBV;
                            break;
                        case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
                            classType = RootSignatureUserClassType::Sampler;
                            break;
                    }

                    // Account for unbounded ranges
                    if (range.NumDescriptors == UINT_MAX) {
                        // Create at space[base + idx]
                        RootSignatureUserMapping& user = GetRootMapping(mapping, classType, range.RegisterSpace, range.BaseShaderRegister);
                        user.rootParameter = i;
                        user.offset = descriptorOffset;
                        user.isUnbounded = true;

                        // Next!
                        ASSERT(rangeIdx + 1 == parameter.DescriptorTable.NumDescriptorRanges, "Unbounded range must be last slot");
                    } else {
                        // Create a mapping per internal register
                        for (uint32_t registerIdx = 0; registerIdx < range.NumDescriptors; registerIdx++) {
                            // Create at space[base + idx]
                            RootSignatureUserMapping& user = GetRootMapping(mapping, classType, range.RegisterSpace, range.BaseShaderRegister + registerIdx);
                            user.rootParameter = i;
                            user.offset = descriptorOffset;
                        }

                        // Next!
                        descriptorOffset += range.NumDescriptors;
                    }
                }

                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS: {
                // Add to hash
                CombineHash(mapping->signatureHash, BufferCRC64(&parameter.Constants, sizeof(parameter.Constants)));

                // Create mapping
                RootSignatureUserMapping& user = GetRootMapping(mapping, RootSignatureUserClassType::CBV, parameter.Constants.RegisterSpace, parameter.Constants.ShaderRegister);
                user.rootParameter = i;
                user.offset = 0;
                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_CBV: {
                // Add to hash
                CombineHash(mapping->signatureHash, BufferCRC64(&parameter.Descriptor, sizeof(parameter.Descriptor)));

                // Create mapping
                RootSignatureUserMapping& user = GetRootMapping(mapping, RootSignatureUserClassType::CBV, parameter.Descriptor.RegisterSpace, parameter.Descriptor.ShaderRegister);
                user.rootParameter = i;
                user.offset = 0;
                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_SRV: {
                // Add to hash
                CombineHash(mapping->signatureHash, BufferCRC64(&parameter.Descriptor, sizeof(parameter.Descriptor)));

                // Create mapping
                RootSignatureUserMapping& user = GetRootMapping(mapping, RootSignatureUserClassType::SRV, parameter.Descriptor.RegisterSpace, parameter.Descriptor.ShaderRegister);
                user.rootParameter = i;
                user.offset = 0;
                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_UAV: {
                // Add to hash
                CombineHash(mapping->signatureHash, BufferCRC64(&parameter.Descriptor, sizeof(parameter.Descriptor)));

                // Create mapping
                RootSignatureUserMapping& user = GetRootMapping(mapping, RootSignatureUserClassType::UAV, parameter.Descriptor.RegisterSpace, parameter.Descriptor.ShaderRegister);
                user.rootParameter = i;
                user.offset = 0;
                break;
            }
        }
    }

    return mapping;
}

template<typename T>
HRESULT SerializeRootSignature(DeviceState* state, D3D_ROOT_SIGNATURE_VERSION version, const T& source, ID3DBlob** out, RootRegisterBindingInfo* outRoot, RootSignaturePhysicalMapping** outMapping, ID3DBlob** outError) {
    *outRoot = GetBindingInfo(state, source);

    // Types
    using Parameter = std::remove_const_t<std::remove_pointer_t<decltype(T::pParameters)>>;
    using DescriptorTable = decltype(Parameter::DescriptorTable);
    using Range = std::remove_const_t<std::remove_pointer_t<decltype(DescriptorTable::pDescriptorRanges)>>;

    // Number of parameters
    uint32_t parameterCount = source.NumParameters + 3u;

    // Copy parameters
    auto* parameters = ALLOCA_ARRAY(Parameter, parameterCount);
    std::memcpy(parameters, source.pParameters, sizeof(Parameter) * source.NumParameters);

    // TODO: Root signatures need to be recompiled on the fly as well, to avoid needless worst-case cost

    // Base ranges
    Range ranges[] = {
        // Shader export range
        {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
            .NumDescriptors = 1u + state->exportHost->GetBound(),
            .BaseShaderRegister = outRoot->shaderExportBaseRegister,
            .RegisterSpace = outRoot->space,
            .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
        },

        // PRMT range
        {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .NumDescriptors = 1u,
            .BaseShaderRegister = outRoot->prmtBaseRegister,
            .RegisterSpace = outRoot->space,
            .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
        },

        // Shader Data range
        {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
            .NumDescriptors = outRoot->shaderResourceCount,
            .BaseShaderRegister = outRoot->shaderResourceBaseRegister,
            .RegisterSpace = outRoot->space,
            .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
        }
    };

    // Shader export parameter
    Parameter& exportParameter = parameters[source.NumParameters + 0u] = {};
    exportParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    exportParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    exportParameter.DescriptorTable.NumDescriptorRanges = 3u;
    exportParameter.DescriptorTable.pDescriptorRanges = ranges;

    // Range version 1.1 assumes STATIC registers, explicitly say otherwise
    if constexpr(std::is_same_v<T, D3D12_ROOT_SIGNATURE_DESC1>) {
        ranges[0].Flags |= D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
    }

    // Descriptor constant parameter
    Parameter& descriptorParameter = parameters[source.NumParameters + 1u] = {};
    descriptorParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    descriptorParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    descriptorParameter.Descriptor.ShaderRegister = outRoot->descriptorConstantBaseRegister;
    descriptorParameter.Descriptor.RegisterSpace = outRoot->space;

    // Get number of events
    uint32_t eventCount{0};
    state->shaderDataHost->Enumerate(&eventCount, nullptr, ShaderDataType::Event);

    // Event constant parameter
    Parameter& eventParameter = parameters[source.NumParameters + 2u] = {};
    eventParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    eventParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    eventParameter.Constants.ShaderRegister = outRoot->eventConstantBaseRegister;
    eventParameter.Constants.RegisterSpace = outRoot->space;
    eventParameter.Constants.Num32BitValues = eventCount;

    // Create mappings
    *outMapping = CreateRootPhysicalMappings(state, parameters, parameterCount);

    // Versioned creation info
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC versioned;
    versioned.Version = version;

    // Copy signatures
    if constexpr(std::is_same_v<T, D3D12_ROOT_SIGNATURE_DESC1>) {
        versioned.Desc_1_1 = source;
        versioned.Desc_1_1.pParameters = parameters;
        versioned.Desc_1_1.NumParameters = parameterCount;
    } else {
        versioned.Desc_1_0 = source;
        versioned.Desc_1_0.pParameters = parameters;
        versioned.Desc_1_0.NumParameters = parameterCount;
    }

    // Create it
    return D3D12SerializeVersionedRootSignature(
        &versioned,
        out,
        outError
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
    uint32_t userRootCount{0};

    // Mapping
    RootSignaturePhysicalMapping* mapping{nullptr};

    // Attempt to re-serialize
    ID3DBlob* serialized{nullptr};
    switch (unconverted->Version) {
        default:
            ASSERT(false, "Invalid root signature version");
            return E_INVALIDARG;
        case D3D_ROOT_SIGNATURE_VERSION_1: {
            userRootCount = unconverted->Desc_1_0.NumParameters;

#ifndef NDEBUG
            hr = SerializeRootSignature(table.state, D3D_ROOT_SIGNATURE_VERSION_1, unconverted->Desc_1_0, &serialized, &bindingInfo, &mapping, &error);
#else // NDEBUG
            hr = SerializeRootSignature(table.state, D3D_ROOT_SIGNATURE_VERSION_1, unconverted->Desc_1_0, &serialized, &bindingInfo, &mapping, nullptr);
#endif // NDEBUG
            break;
        }
        case D3D_ROOT_SIGNATURE_VERSION_1_1: {
            userRootCount = unconverted->Desc_1_1.NumParameters;

#ifndef NDEBUG
            hr = SerializeRootSignature(table.state, D3D_ROOT_SIGNATURE_VERSION_1_1, unconverted->Desc_1_1, &serialized, &bindingInfo, &mapping, &error);
#else // NDEBUG
            hr = SerializeRootSignature(table.state, D3D_ROOT_SIGNATURE_VERSION_1_1, unconverted->Desc_1_1, &serialized, &bindingInfo, &mapping, nullptr);
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
    auto* state = new (table.state->allocators, kAllocState) RootSignatureState();
    state->allocators = table.state->allocators;
    state->parent = device;
    state->rootBindingInfo = bindingInfo;
    state->userRootCount = userRootCount;
    state->physicalMapping = mapping;
    state->object = rootSignature;

    // Create detours
    rootSignature = CreateDetour(state->allocators, rootSignature, state);

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

HRESULT HookID3D12RootSignatureGetDevice(ID3D12RootSignature *_this, const IID &riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevice);
}

RootSignatureState::~RootSignatureState() {

}
