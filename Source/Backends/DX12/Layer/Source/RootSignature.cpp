// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
RootRegisterBindingInfo GetBindingInfo(DeviceState* state, const T& source, RootSignatureLogicalMapping* outLogical) {
    uint32_t userRegisterSpaceBound = 0;

    // Preallocate
    outLogical->userRootCount = source.NumParameters;
    outLogical->userRootHeapTypes.resize(source.NumParameters);
    
    // Get the user bound
    for (uint32_t i = 0; i < source.NumParameters; i++) {
        const auto& parameter = source.pParameters[i];
        switch (parameter.ParameterType) {
            case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE: {
                for (uint32_t j = 0; j < parameter.DescriptorTable.NumDescriptorRanges; j++) {
                    const auto& range = parameter.DescriptorTable.pDescriptorRanges[j];
                    userRegisterSpaceBound = std::max(userRegisterSpaceBound, range.RegisterSpace + 1);
                }

                // Assign heap type from first range
                if (parameter.DescriptorTable.NumDescriptorRanges > 0) {
                    switch (parameter.DescriptorTable.pDescriptorRanges[0].RangeType) {
                        default:
                            ASSERT(false, "INvalid descriptor range type");
                        case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                        case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                        case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
                            outLogical->userRootHeapTypes[i] = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                            break;
                        case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER: 
                            outLogical->userRootHeapTypes[i] = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
                            break;
                    }
                } else {
                    outLogical->userRootHeapTypes[i] = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
                }
                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS: {
                outLogical->userRootHeapTypes[i] = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
                userRegisterSpaceBound = std::max(userRegisterSpaceBound, parameter.Constants.RegisterSpace + 1);
                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_CBV:
            case D3D12_ROOT_PARAMETER_TYPE_SRV: {
                outLogical->userRootHeapTypes[i] = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                userRegisterSpaceBound = std::max(userRegisterSpaceBound, parameter.Descriptor.RegisterSpace + 1);
                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_UAV: {
                outLogical->userRootHeapTypes[i] = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
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

    // Set base register for resource prmt data
    bindingInfo.resourcePRMTBaseRegister = registerOffset;
    registerOffset += 1u;

    // Set base register for sampler prmt data
    bindingInfo.samplerPRMTBaseRegister = registerOffset;
    registerOffset += 1u;

    // Set base register for shader data constants
    bindingInfo.shaderDataConstantRegister = registerOffset;
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

void WriteRootVisibilityMapping(RootSignaturePhysicalMapping* mapping, RootSignatureUserClassType type, RootParameterVisibility visibility, uint32_t space, uint32_t offset, const RootSignatureUserMapping& value) {
    // TODO: This is a lot of indirections, perhaps a linear approach is more favorable?

    // Get visibility class
    RootSignatureVisibilityClass& visibilityClass = mapping->visibility[static_cast<uint32_t>(visibility)];

    // Get user class
    RootSignatureUserClass& userClass = visibilityClass.spaces[static_cast<uint32_t>(type)];

    // Ensure user space length
    if (userClass.spaces.size() <= space) {
        userClass.spaces.resize(space + 1);
    }

    // Get user space
    RootSignatureUserSpace& userSpace = userClass.spaces[space];

    // Ensure mappings length
    if (userSpace.mappings.size() <= offset) {
        userSpace.mappings.resize(offset + 1);
    }

    // Write mapping
    userSpace.mappings[offset] = value;
}

void WriteRootMapping(RootSignaturePhysicalMapping* mapping, RootSignatureUserClassType type, D3D12_SHADER_VISIBILITY visibility, uint32_t space, uint32_t offset, const RootSignatureUserMapping& value) {
    if (visibility == D3D12_SHADER_VISIBILITY_ALL) {
        // Write to all visibilities
        for (uint32_t i = 0; i < static_cast<uint32_t>(RootParameterVisibility::Count); i++) {
            WriteRootVisibilityMapping(mapping, type, static_cast<RootParameterVisibility>(i), space, offset, value);
        }
    } else {
        // Translate local visibility
        RootParameterVisibility localVisibility;
        switch (visibility) {
            default:
                ASSERT(false, "Invalid visibility");
                localVisibility = RootParameterVisibility::Vertex;
                break;
            case D3D12_SHADER_VISIBILITY_VERTEX:
                localVisibility = RootParameterVisibility::Vertex;
                break;
            case D3D12_SHADER_VISIBILITY_HULL:
                localVisibility = RootParameterVisibility::Hull;
                break;
            case D3D12_SHADER_VISIBILITY_DOMAIN:
                localVisibility = RootParameterVisibility::Domain;
                break;
            case D3D12_SHADER_VISIBILITY_GEOMETRY:
                localVisibility = RootParameterVisibility::Geometry;
                break;
            case D3D12_SHADER_VISIBILITY_PIXEL:
                localVisibility = RootParameterVisibility::Pixel;
                break;
        }

        // Write value
        WriteRootVisibilityMapping(mapping, type, localVisibility, space, offset, value);
    }
}

template<typename T>
static RootSignaturePhysicalMapping* CreateRootPhysicalMappings(DeviceState* state, const T* parameters, uint32_t parameterCount, const D3D12_STATIC_SAMPLER_DESC* staticSamplers, uint32_t staticSamplerCount) {
    auto* mapping = new (state->allocators, kAllocStateRootSignature) RootSignaturePhysicalMapping;

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
                CombineHash(mapping->signatureHash, BufferCRC32Short(parameter.DescriptorTable.pDescriptorRanges, sizeof(D3D12_DESCRIPTOR_RANGE1) * parameter.DescriptorTable.NumDescriptorRanges));

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
                        RootSignatureUserMapping user;
                        user.rootParameter = i;
                        user.offset = descriptorOffset;
                        user.isUnbounded = true;
                        WriteRootMapping(mapping, classType, parameter.ShaderVisibility, range.RegisterSpace, range.BaseShaderRegister, user);

                        // Next!
                        ASSERT(rangeIdx + 1 == parameter.DescriptorTable.NumDescriptorRanges, "Unbounded range must be last slot");
                    } else {
                        // Create a mapping per internal register
                        for (uint32_t registerIdx = 0; registerIdx < range.NumDescriptors; registerIdx++) {
                            // Create at space[base + idx]
                            RootSignatureUserMapping user;
                            user.rootParameter = i;
                            user.offset = descriptorOffset + registerIdx;
                            WriteRootMapping(mapping, classType, parameter.ShaderVisibility, range.RegisterSpace, range.BaseShaderRegister + registerIdx, user);
                        }

                        // Next!
                        descriptorOffset += range.NumDescriptors;
                    }
                }

                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS: {
                // Add to hash
                CombineHash(mapping->signatureHash, BufferCRC32Short(&parameter.Constants, sizeof(parameter.Constants)));

                // Create mapping
                RootSignatureUserMapping user;
                user.isRootResourceParameter = true;
                user.rootParameter = i;
                user.offset = 0;
                WriteRootMapping(mapping, RootSignatureUserClassType::CBV, parameter.ShaderVisibility, parameter.Constants.RegisterSpace, parameter.Constants.ShaderRegister, user);
                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_CBV: {
                // Add to hash
                CombineHash(mapping->signatureHash, BufferCRC32Short(&parameter.Descriptor, sizeof(parameter.Descriptor)));

                // Create mapping
                RootSignatureUserMapping user;
                user.isRootResourceParameter = true;
                user.rootParameter = i;
                user.offset = 0;
                WriteRootMapping(mapping, RootSignatureUserClassType::CBV, parameter.ShaderVisibility, parameter.Descriptor.RegisterSpace, parameter.Descriptor.ShaderRegister, user);
                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_SRV: {
                // Add to hash
                CombineHash(mapping->signatureHash, BufferCRC32Short(&parameter.Descriptor, sizeof(parameter.Descriptor)));

                // Create mapping
                RootSignatureUserMapping user;
                user.isRootResourceParameter = true;
                user.rootParameter = i;
                user.offset = 0;
                WriteRootMapping(mapping, RootSignatureUserClassType::SRV, parameter.ShaderVisibility, parameter.Descriptor.RegisterSpace, parameter.Descriptor.ShaderRegister, user);
                break;
            }
            case D3D12_ROOT_PARAMETER_TYPE_UAV: {
                // Add to hash
                CombineHash(mapping->signatureHash, BufferCRC32Short(&parameter.Descriptor, sizeof(parameter.Descriptor)));

                // Create mapping
                RootSignatureUserMapping user;
                user.isRootResourceParameter = true;
                user.rootParameter = i;
                user.offset = 0;
                WriteRootMapping(mapping, RootSignatureUserClassType::UAV, parameter.ShaderVisibility, parameter.Descriptor.RegisterSpace, parameter.Descriptor.ShaderRegister, user);
                break;
            }
        }
    }

    // Create static sampler mappings
    for (uint32_t i = 0; i < staticSamplerCount; i++) {
        const D3D12_STATIC_SAMPLER_DESC& sampler = staticSamplers[i];

        // Add to hash
        CombineHash(mapping->signatureHash, BufferCRC32Short(&sampler, sizeof(sampler)));

        // Create mapping
        RootSignatureUserMapping user;
        user.isRootResourceParameter = true;
        user.isStaticSampler = true;
        user.rootParameter = i;
        user.offset = 0;
        WriteRootMapping(mapping, RootSignatureUserClassType::Sampler, sampler.ShaderVisibility, sampler.RegisterSpace, sampler.ShaderRegister, user);
    }

    // OK
    return mapping;
}

template<typename T>
HRESULT SerializeRootSignature(DeviceState* state, D3D_ROOT_SIGNATURE_VERSION version, const T& source, ID3DBlob** out, RootRegisterBindingInfo* outRoot, RootSignatureLogicalMapping* outLogical, RootSignaturePhysicalMapping** outMapping, ID3DBlob** outError) {
    *outRoot = GetBindingInfo(state, source, outLogical);
    
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

        // Resource PRMT range
        {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .NumDescriptors = 1u,
            .BaseShaderRegister = outRoot->resourcePRMTBaseRegister,
            .RegisterSpace = outRoot->space,
            .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
        },

        // Sampler PRMT range
        {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .NumDescriptors = 1u,
            .BaseShaderRegister = outRoot->samplerPRMTBaseRegister,
            .RegisterSpace = outRoot->space,
            .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
        },

        // Constant range
        {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            .NumDescriptors = 1u,
            .BaseShaderRegister = outRoot->shaderDataConstantRegister,
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
    exportParameter.DescriptorTable.NumDescriptorRanges = 5u;
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
    *outMapping = CreateRootPhysicalMappings(state, parameters, parameterCount, source.pStaticSamplers, source.NumStaticSamplers);

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

    // Signature to the users specification
    ID3D12RootSignature* nativeRootSignature{nullptr};
    
    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateRootSignature(table.next, nodeMask, blob, length, __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&nativeRootSignature));
    if (FAILED(hr)) {
        return hr;
    }

    // Temporary deserializer for re-serialization
    ID3D12VersionedRootSignatureDeserializer* deserializer{nullptr};

    // Immediate deserialization
    hr = D3D12CreateVersionedRootSignatureDeserializer(blob, length, IID_PPV_ARGS(&deserializer));
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

    // Logical mapping
    RootSignatureLogicalMapping logicalMapping;
    
    // Mapping
    RootSignaturePhysicalMapping* mapping{nullptr};

    // Object
    ID3D12RootSignature* rootSignature{nullptr};

    // Attempt to re-serialize
    ID3DBlob* serialized{nullptr};
    switch (unconverted->Version) {
        default:
            ASSERT(false, "Invalid root signature version");
        return E_INVALIDARG;
        case D3D_ROOT_SIGNATURE_VERSION_1: {
#ifndef NDEBUG
            hr = SerializeRootSignature(table.state, D3D_ROOT_SIGNATURE_VERSION_1, unconverted->Desc_1_0, &serialized, &bindingInfo, &logicalMapping, &mapping, &error);
#else // NDEBUG
            hr = SerializeRootSignature(table.state, D3D_ROOT_SIGNATURE_VERSION_1, unconverted->Desc_1_0, &serialized, &bindingInfo, &logicalMapping, &mapping, nullptr);
#endif // NDEBUG
        break;
        }
        case D3D_ROOT_SIGNATURE_VERSION_1_1: {
#ifndef NDEBUG
            hr = SerializeRootSignature(table.state, D3D_ROOT_SIGNATURE_VERSION_1_1, unconverted->Desc_1_1, &serialized, &bindingInfo, &logicalMapping, &mapping, &error);
#else // NDEBUG
            hr = SerializeRootSignature(table.state, D3D_ROOT_SIGNATURE_VERSION_1_1, unconverted->Desc_1_1, &serialized, &bindingInfo, &logicalMapping, &mapping, nullptr);
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

    // Pass down callchain
    hr = table.bottom->next_CreateRootSignature(table.next, nodeMask, serialized->GetBufferPointer(), serialized->GetBufferSize(), __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&rootSignature));
    if (FAILED(hr)) {
        serialized->Release();
        return hr;
    }

    // Cleanup
    serialized->Release();

    // Cleanup
    deserializer->Release();

    // Create state
    auto* state = new (table.state->allocators, kAllocStateRootSignature) RootSignatureState();
    state->allocators = table.state->allocators;
    state->parent = device;
    state->rootBindingInfo = bindingInfo;
    state->logicalMapping = std::move(logicalMapping);
    state->physicalMapping = mapping;
    state->object = rootSignature;
    state->nativeObject = nativeRootSignature;

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
    nativeObject->Release();
}
