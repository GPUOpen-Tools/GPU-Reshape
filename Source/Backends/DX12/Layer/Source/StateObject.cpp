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

// Layer
#include <Backends/DX12/PipelineSubObjectReader.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>
#include <Backends/DX12/Compiler/DXBC/DXBCUtils.h>
#include <Backends/DX12/Compiler/DXBC/DXBCHeader.h>
#include <Backends/DX12/Compiler/DXBC/DXBCRDATRootSignature.h>
#include <Backends/DX12/Controllers/InstrumentationController.h>
#include <Backends/DX12/States/ShaderState.h>
#include <Backends/DX12/Pipeline.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/States/StateObjectState.h>
#include <Backends/DX12/StateSubObjectWriter.h>

// Shared
#include <Shared/ShaderRecordPatching.h>

// Common
#include <Common/String.h>

struct StateObjectInlineAssociationEntry {
    /// Name of this inline export
    LPCWSTR name{nullptr};

    /// Assigned sub-object index
    uint32_t subObjectIndex = 0;
};

struct StateObjectInlineAssociation {
    /// All inlined sub-objects
    TrivialStackVector<StateObjectInlineAssociationEntry, 4u> entries;
};

struct StateObjectExportEntry {
    /// DXBC name
    std::wstring exportName;

    /// Associated name
    std::wstring name;
};

struct StateObjectInlineExport {
    /// Name of the inline export
    LPCWSTR name;

    /// Sub-object to be inlined
    DXBCSubObjectExport subObject;
};

struct StateObjectEntry {
    /// Write index for associations
    uint32_t index{UINT32_MAX};

    /// Has this been associated?
    bool associated = false;
};

struct StateObjectCache {
    /// Metadata for original objects
    std::unordered_map<const D3D12_STATE_SUBOBJECT*, StateObjectEntry> subObjects;
};

/// Prototypes
void CreateStateSubObject(const DeviceTable& table, StateObjectState * state, const D3D12_STATE_SUBOBJECT* subObject, StateObjectCache& cache);
void CreateStateSubObjects(const DeviceTable& table, StateObjectState* state, const D3D12_STATE_OBJECT_DESC* pDesc);

static RootSignatureState* GetLocalRootSignatureForIdentifier(StateObjectState* state, LPCWSTR _export) {
    StateSubObjectIndex index = state->subObjectMap.at(_export);

    switch (index.type) {
        default: {
            ASSERT(false, "Invalid type");
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
            const D3D12_HIT_GROUP_DESC &hitGroup = state->hitGroupSubobjects[index.index];

            if (hitGroup.IntersectionShaderImport) {
                return GetLocalRootSignatureForIdentifier(state, hitGroup.IntersectionShaderImport);
            }
                            
            if (hitGroup.ClosestHitShaderImport) {
                return GetLocalRootSignatureForIdentifier(state, hitGroup.ClosestHitShaderImport);
            }
                            
            if (hitGroup.AnyHitShaderImport) {
                return GetLocalRootSignatureForIdentifier(state, hitGroup.AnyHitShaderImport);
            }
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY: {
            StateShaderSubObject& stateSubObject = state->shaderSubObjects[index.index];

            // Find the specific export in this library
            for (StateShaderSubObjectExport& subObjectExport : stateSubObject.functionExports) {
                if (subObjectExport.name == _export) {
                    return subObjectExport.localSignature;
                }
            }
            break;
        }
    }

    // Not found
    return nullptr;
}

static void CreateStateObjectIdentifierTable(const DeviceTable& table, StateObjectState* state) {
    state->identifierTable = new StateObjectShaderIdentifierTable();
    
    // Give the keys some breathing room, trade off between memory and linear probing
    state->identifierTable->tableCount = static_cast<uint64_t>(state->identifierExports.size() * 1.5f);

    // Create table
    D3D12_RESOURCE_DESC tableDesc{};
    tableDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    tableDesc.Alignment = 0;
    tableDesc.Width = state->identifierTable->tableCount * sizeof(StateObjectShaderIdentifierTableEntry);
    tableDesc.Height = 1;
    tableDesc.DepthOrArraySize = 1;
    tableDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    tableDesc.Format = DXGI_FORMAT_UNKNOWN;
    tableDesc.MipLevels = 1;
    tableDesc.SampleDesc.Quality = 0;
    tableDesc.SampleDesc.Count = 1;
    state->identifierTable->tableAllocation = table.state->deviceAllocator->Allocate(tableDesc, AllocationResidency::Host);

    // Create linear list
    D3D12_RESOURCE_DESC listDesc{};
    listDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    listDesc.Alignment = 0;
    listDesc.Width = state->identifierExports.size() * sizeof(SBTIdentifierTableEntry);
    listDesc.Height = 1;
    listDesc.DepthOrArraySize = 1;
    listDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    listDesc.Format = DXGI_FORMAT_UNKNOWN;
    listDesc.MipLevels = 1;
    listDesc.SampleDesc.Quality = 0;
    listDesc.SampleDesc.Count = 1;
    state->identifierTable->listAllocation = table.state->deviceAllocator->Allocate(listDesc, AllocationResidency::Host);

    // Map table
    D3D12_RANGE tableRange;
    tableRange.Begin = 0;
    tableRange.End = tableDesc.Width;
    state->identifierTable->tableAllocation.resource->Map(0, &tableRange, reinterpret_cast<void**>(&state->identifierTable->table));

    // Map linear list
    D3D12_RANGE listRange;
    listRange.Begin = 0;
    listRange.End = listDesc.Width;
    state->identifierTable->listAllocation.resource->Map(0, &listRange, reinterpret_cast<void **>(&state->identifierTable->list));

    // Initialize the table
    for (uint32_t i = 0; i < state->identifierTable->tableCount; i++) {
        state->identifierTable->table[i] = StateObjectShaderIdentifierTableEntry{};
    }

    // Get properties for shader identifier queries
    ID3D12StateObjectProperties* properties{nullptr};
    state->object->QueryInterface(__uuidof(ID3D12StateObjectProperties), reinterpret_cast<void**>(&properties));

    // All identifiers
    std::vector<SBTIdentifierTableEntry> identifiers;

    // Current allocation index
    uint32_t indexAllocation = 0;

    // Process all exported identifiers
    for (const std::wstring& name : state->identifierExports) {
        // Get data
        const void* identifierData = properties->GetShaderIdentifier(name.c_str());
        ASSERT(identifierData, "Failed to retrieve data");

        // Just in case
        // Really all identifiers are callable, but let's keep things safe
        if (!identifierData) {
            continue;
        }
        
        // The index assigned to this export, must be 1:1 to identifierExports
        SBTIdentifierTableEntry& entry = identifiers.emplace_back();
        entry.Index = indexAllocation++;
        entry.SBTDWords = 0;
        std::memset(entry.SBTSourceParameters, 0u, sizeof(entry.SBTSourceParameters));

        // Copy identifier dwords
        static_assert(sizeof(entry.Identifier.DWords) == D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, "Unexpected identifier size");
        std::memcpy(&entry.Identifier.DWords, identifierData, sizeof(entry.Identifier.DWords));

        // Hash the identifier
        entry.Key = ShaderIdentifierHash(entry.Identifier);

        // The state object being queried
        RootSignatureState* localRootSignature = GetLocalRootSignatureForIdentifier(state, name.c_str());

        // Create local root signature addressing masks
        if (localRootSignature) {
            // Number of dwords, mostly used for appending things
            entry.SBTDWords = localRootSignature->physicalMapping->rootDescriptorDWordCount;
            entry.ParameterCount = localRootSignature->logicalMapping.userRootCount;

            // Create vaddr masks for resource and sampler spaces
            for (uint32_t i = 0; i < localRootSignature->logicalMapping.userRootCount; i++) {
                const RootSignatureRootMapping &mapping = localRootSignature->logicalMapping.userRootMappings[i];

                switch (mapping.type) {
                    case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE: {
                        entry.SBTSourceParameters[i] = SBTRootParameterTypeInfo {
                            .type = static_cast<uint32_t>(SBTRootParameterType::VAddr64),
                            .prmtOffset = localRootSignature->physicalMapping->rootDWordOffsets[i],
                            .payload = 0
                        };
                        break;
                    }
                    case D3D12_ROOT_PARAMETER_TYPE_CBV:
                    case D3D12_ROOT_PARAMETER_TYPE_UAV:
                    case D3D12_ROOT_PARAMETER_TYPE_SRV: {
                        entry.SBTSourceParameters[i] = SBTRootParameterTypeInfo {
                            .type = static_cast<uint32_t>(SBTRootParameterType::InlinePRM),
                            .prmtOffset = localRootSignature->physicalMapping->rootDWordOffsets[i],
                            .payload = 0
                        };
                        break;
                    }
                    case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS: {
                        entry.SBTSourceParameters[i] = SBTRootParameterTypeInfo {
                            .type = static_cast<uint32_t>(SBTRootParameterType::Constant),
                            .prmtOffset = localRootSignature->physicalMapping->rootDWordOffsets[i],
                            .payload = mapping.inlineDwordCount
                        };
                        break;
                    }
                }
            }
        }

        // Increment table entry count
        state->identifierTable->table[entry.Key % state->identifierTable->tableCount].end++;
    }

    // Current allocation offset
    uint32_t allocationOffset = 0;

    // Create table starting ranges, end becomes the in-list allocation counter
    for (uint64_t i = 0; i < state->identifierTable->tableCount; i++) {
        StateObjectShaderIdentifierTableEntry &entry = state->identifierTable->table[i];
        if (!entry.end) {
            continue;
        }

        // Set indices
        uint32_t count = entry.end;
        entry.start = allocationOffset;
        entry.end = allocationOffset;
        allocationOffset += count;
    }

    ASSERT(allocationOffset == state->identifierExports.size(), "Unexpected allocation to export count");

    // Finally, insert all identifiers
    for (const SBTIdentifierTableEntry& identifier : identifiers) {
        StateObjectShaderIdentifierTableEntry &entry = state->identifierTable->table[identifier.Key % state->identifierTable->tableCount];
        state->identifierTable->list[entry.end++] = identifier;
    }

#ifndef NDEBUG
    // Validate that we never exceeded the in-list bounds, otherwise we overlap!
    for (uint64_t i = 0; i < state->identifierTable->tableCount; i++) {
        StateObjectShaderIdentifierTableEntry &entry = state->identifierTable->table[i];
        if (entry.start == UINT32_MAX) {
            continue;
        }

        uint64_t nextStart = UINT32_MAX;
        for (uint64_t j = i + 1; j < state->identifierTable->tableCount && nextStart == UINT32_MAX; j++) {
            nextStart = state->identifierTable->table[j].start;
        }

        ASSERT(nextStart == UINT32_MAX || entry.end == nextStart, "Stomped identifier ranges");
    }
#endif // NDEBUG

    // Cleanup
    properties->Release();
}

StateObjectShaderIdentifierPatch* CreateStateObjectShaderIdentifierPatch(StateObjectState* state, ID3D12StateObject* stateObject) {
    auto table = GetTable(state->parent);

    // Create a new patch object
    auto* patch = new (state->allocators) StateObjectShaderIdentifierPatch;
    patch->count = state->identifierExports.size();

    // Create the linear patch list
    D3D12_RESOURCE_DESC listDesc{};
    listDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    listDesc.Alignment = 0;
    listDesc.Width = state->identifierExports.size() * sizeof(SBTIdentifierTableEntry);
    listDesc.Height = 1;
    listDesc.DepthOrArraySize = 1;
    listDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    listDesc.Format = DXGI_FORMAT_UNKNOWN;
    listDesc.MipLevels = 1;
    listDesc.SampleDesc.Quality = 0;
    listDesc.SampleDesc.Count = 1;
    patch->listAllocation = table.state->deviceAllocator->Allocate(listDesc, AllocationResidency::Host);

    // Map it
    D3D12_RANGE tableRange;
    tableRange.Begin = 0;
    tableRange.End = listDesc.Width;
    patch->listAllocation.resource->Map(0, &tableRange, reinterpret_cast<void**>(&patch->list));

    // Get properties for shader identifier queries
    ID3D12StateObjectProperties* properties{nullptr};
    stateObject->QueryInterface(__uuidof(ID3D12StateObjectProperties), reinterpret_cast<void**>(&properties));

    // Current allocation index
    uint32_t indexAllocation = 0;

    // Process all exported identifiers
    for (const std::wstring& name : state->identifierExports) {
        SBTIdentifierPatch &entry = patch->list[indexAllocation++];

        // Get identifier data
        const void* identifierData = properties->GetShaderIdentifier(name.c_str());
        ASSERT(identifierData, "Failed to retrieve data");

        // Just in case
        if (!identifierData) {
            continue;
        }

        // Mark as instrumented
        // TODO[rt]: Actually detect if it's a carry on or instrumented
        entry.Metadata |= SBTIdentifierPatchFlagInstrumented;

        // Copy all identifier dwords
        static_assert(sizeof(entry.Patch.DWords) == D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, "Unexpected identifier size");
        std::memcpy(&entry.Patch.DWords, identifierData, sizeof(entry.Patch.DWords));
    }

    // Cleanup
    properties->Release();

    // OK
    return patch;
}

static StateShaderSubObjectExport* FindStateObjectExport(StateShaderSubObject& shader, LPCWSTR name) {
    for (StateShaderSubObjectExport& _export : shader.functionExports) {
        if (_export.name == name) {
            return &_export;
        }
    }

    ASSERT(false, "Failed to find export");
    return nullptr;
}

static ID3D12RootSignature* CreateInlineSubObjectRootSignature(StateObjectState* state, const DXBCSubObjectExport& _export) {
    // First, serialize it into a runtime format
    ID3DBlob* serialized;
    CreateDXBCRDATRootSignature(_export.rootSignature.data, _export.rootSignature.length, &serialized);
    ASSERT(serialized, "Failed to serialize inline signature");

    // Create inline signature
    // Note, this is done *through* the wrapped device
    ID3D12RootSignature* signature;
    state->parent->CreateRootSignature(0, serialized->GetBufferPointer(), serialized->GetBufferSize(), __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&signature));
    ASSERT(signature, "Failed to create inline signature");

    // State object owns it
    state->inlinedSubObjectStates.push_back(signature);
    return signature;
}

static void CreateStateObjectInlineSubStream(StateObjectState* state, StateSubObjectWriter& writer, const StateObjectInlineExport& inlineExport, StateObjectInlineAssociation& association) {
    // Keep an entry for later association
    association.entries.Add(StateObjectInlineAssociationEntry {
        .name = inlineExport.name,
        .subObjectIndex = static_cast<uint32_t>(writer.SubObjectCount())
    });

    // Handle object
    // We're not actually unwrapping it, but rather creating a regular stream for it
    switch (inlineExport.subObject.subObjectKind) {
        case DXBCRuntimeDataSubObjectKind::StateObjectConfig: {
            writer.Add(D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG, inlineExport.subObject.config);
            break;
        }
        case DXBCRuntimeDataSubObjectKind::GlobalRootSignature: {
            writer.Add(D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, D3D12_GLOBAL_ROOT_SIGNATURE {
                .pGlobalRootSignature = CreateInlineSubObjectRootSignature(state, inlineExport.subObject)
            });
            break;
        }
        case DXBCRuntimeDataSubObjectKind::LocalRootSignature: {
            writer.Add(D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, D3D12_LOCAL_ROOT_SIGNATURE {
                .pLocalRootSignature = CreateInlineSubObjectRootSignature(state, inlineExport.subObject)
            });
            break;
        }
        case DXBCRuntimeDataSubObjectKind::SubObjectToExportsAssociation: {
            // Copy over all export strings
            TrivialStackVector<LPCWSTR, 4u> exports;
            for (uint32_t i = 0; i < inlineExport.subObject.subObjectToExportsAssociation.exportView.indexCount; i++) {
                exports.Add(writer.EmbedAnsi(inlineExport.subObject.subObjectToExportsAssociation.exportView[i]));
            }

            // Find the referenced sub-object by name
            uint32_t subObjectIndex = UINT32_MAX;
            for (const StateObjectInlineAssociationEntry& entry : association.entries) {
                if (std::wcac_equals(entry.name, inlineExport.subObject.subObjectToExportsAssociation.subObject)) {
                    subObjectIndex = entry.subObjectIndex;
                    break;
                }
            }

            // Associate!
            ASSERT(subObjectIndex != UINT32_MAX, "Failed to associate sub-object index");
            writer.SubObjectAssociation(exports.Data(), static_cast<uint32_t>(exports.Size()), subObjectIndex);
            break;
        }
        case DXBCRuntimeDataSubObjectKind::RaytracingShaderConfig: {
            writer.Add(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, inlineExport.subObject.shaderConfig);
            break;
        }
        case DXBCRuntimeDataSubObjectKind::RaytracingPipelineConfig: {
            writer.Add(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, inlineExport.subObject.pipelineConfig);
            break;
        }
        case DXBCRuntimeDataSubObjectKind::HitGroup: {
            writer.Add(D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, D3D12_HIT_GROUP_DESC {
                .HitGroupExport = inlineExport.name,
                .Type = static_cast<D3D12_HIT_GROUP_TYPE>(inlineExport.subObject.hitGroup.type),
                .AnyHitShaderImport = writer.EmbedAnsi(inlineExport.subObject.hitGroup.anyHitExport),
                .ClosestHitShaderImport = writer.EmbedAnsi(inlineExport.subObject.hitGroup.closestHitExport),
                .IntersectionShaderImport = writer.EmbedAnsi(inlineExport.subObject.hitGroup.intersectionExport)
            });
            break;
        }
        case DXBCRuntimeDataSubObjectKind::RaytracingPipelineConfig1: {
            writer.Add(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG1, inlineExport.subObject.pipelineConfig1);
            break;
        }
    }
}

static void CreateStateObjectInlineSubStream(const DeviceTable& table, StateObjectState* state, const TrivialStackVector<StateObjectInlineExport, 4u>& inlineExports) {
    StateObjectInlineAssociation associations;

    // We're writing the inline sub-objects as if it was a normal stream
    StateSubObjectWriter writer(state->allocators);

    // Skip the associations, may be out of order
    for (const StateObjectInlineExport& inlineExport : inlineExports) {
        if (inlineExport.subObject.subObjectKind != DXBCRuntimeDataSubObjectKind::SubObjectToExportsAssociation) {
            CreateStateObjectInlineSubStream(state, writer, inlineExport, associations);
        }
    }

    // Finally, handle the associations
    for (const StateObjectInlineExport& inlineExport : inlineExports) {
        if (inlineExport.subObject.subObjectKind == DXBCRuntimeDataSubObjectKind::SubObjectToExportsAssociation) {
            CreateStateObjectInlineSubStream(state, writer, inlineExport, associations);
        }
    }

    // Finally, pass it down the usual handler
    // Semantically this is the same thing, but greatly simplifies things on this end
    D3D12_STATE_OBJECT_DESC desc = writer.GetDesc(D3D12_STATE_OBJECT_TYPE_COLLECTION);
    CreateStateSubObjects(table, state, &desc);
}

static void SafeAddStateObjectIdentifier(StateObjectState* state, const std::wstring& name) {
    ASSERT(!state->identifierExports.contains(name), "Identifier name double registration");
    state->identifierExports.insert(name);
}

static void AddUniqueStateObjectShader(StateObjectState* state, ShaderState* shader) {
    // TODO[rt]: Consider a secondary slot lookup
    if (std::ranges::find(state->shaders, shader) == state->shaders.end()) {
        state->shaders.push_back(shader);
    }
}

static void RemoveStateObjectExport(StateObjectState* state, LPCWSTR name) {
    // If not mapped, just ignore
    auto it = state->subObjectMap.find(name);
    if (it == state->subObjectMap.end()) {
        return;
    }

    // Handle type
    switch (it->second.type) {
        default: {
            ASSERT(false, "Unknown type");
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
            // Not end of container?
            if (it->second.index != state->hitGroupSubobjects.size() - 1) {
                // Swap with back
                D3D12_HIT_GROUP_DESC back = state->hitGroupSubobjects.back();
                state->hitGroupSubobjects[it->second.index] = back;

                // Update swapped lookup
                state->subObjectMap[back.HitGroupExport] = {
                    .type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,
                    .index = it->second.index
                };
            }

            state->hitGroupSubobjects.pop_back();
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY: {
            StateShaderSubObject& shader = state->shaderSubObjects[it->second.index];

            // Remove the referenced export
            for (size_t i = 0; i < shader.functionExports.size(); i++) {
                StateShaderSubObjectExport &_export = shader.functionExports[i];
                if (_export.name == name) {
                    shader.functionExports.erase(shader.functionExports.begin() + i);
                    break;
                }
            }

            // All exports removed?
            if (shader.functionExports.empty()) {
                // TODO[rt]: Consider a secondary slot lookup
                state->shaders.erase(std::remove(state->shaders.begin(), state->shaders.end(), shader.shader), state->shaders.end());

                // Not end of container?
                if (it->second.index != state->shaderSubObjects.size() - 1) {
                    StateShaderSubObject back = std::move(state->shaderSubObjects.back());

                    // Update swapped lookups
                    for (const StateShaderSubObjectExport& _export : back.functionExports) {
                        state->subObjectMap[_export.name] = {
                            .type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
                            .index = it->second.index
                        };
                    }

                    // Swap with back
                    state->shaderSubObjects[it->second.index] = std::move(back);
                }

                state->shaderSubObjects.pop_back();
            }
            break;
        }
    }

    state->subObjectMap.erase(name);
}

void CreateStateSubObject(const DeviceTable& table, StateObjectState* state, const D3D12_STATE_SUBOBJECT* subObject, StateObjectCache& cache) {
    switch (subObject->Type) {
        default: {
            state->writer.DeepAdd(subObject->Type, subObject->pDesc);

            // Association lookup
            cache.subObjects[subObject].index = static_cast<uint32_t>(state->writer.SubObjectCount()) - 1;
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
            auto contained = StateSubObjectWriter::Read<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION>(*subObject);

            // Mark the sub-object as associated
            cache.subObjects.at(contained.pSubobjectToAssociate).associated = true;

            // All functions to associate
            TrivialStackVector<const wchar_t*, 4u> associatedFunctions;

            // One-to-many
            for (uint32_t exportIndex = 0; exportIndex < contained.NumExports; exportIndex++) {
                LPCWSTR exportName = contained.pExports[exportIndex];

                const StateSubObjectIndex &index = state->subObjectMap.at(exportName);
                switch (index.type) {
                    default: {
                        ASSERT(false, "Invalid type");
                        break;
                    }
                    case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
                        const D3D12_HIT_GROUP_DESC &hitGroup = state->hitGroupSubobjects[index.index];

                        if (hitGroup.IntersectionShaderImport) {
                            associatedFunctions.Add(hitGroup.IntersectionShaderImport);
                        }
                        
                        if (hitGroup.ClosestHitShaderImport) {
                            associatedFunctions.Add(hitGroup.ClosestHitShaderImport);
                        }
                        
                        if (hitGroup.AnyHitShaderImport) {
                            associatedFunctions.Add(hitGroup.AnyHitShaderImport);
                        }
                        break;
                    }
                    case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY: {
                        associatedFunctions.Add(exportName);
                        break;
                    }
                }
            }

            // Now that it's resolved, associate them
            for (LPCWSTR function : associatedFunctions) {
                StateSubObjectIndex subObjectIndex = state->subObjectMap.at(function);
                ASSERT(subObjectIndex.type == D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, "Unexpected index");
                
                // Find the object
                StateShaderSubObject& referencedSubObject = state->shaderSubObjects[subObjectIndex.index];

                // Find export
                StateShaderSubObjectExport* _export = FindStateObjectExport(referencedSubObject, function);
                if (!_export) {
                    continue;
                }

                // Associate the object
                switch (contained.pSubobjectToAssociate->Type) {
                    default: {
                        StateSubObjectAssociation &association = _export->associations.emplace_back();
                        association.type = contained.pSubobjectToAssociate->Type;
                        association.data.Resize(StateSubObjectWriter::GetSize(association.type));
                        std::memcpy(association.data.Data(), contained.pSubobjectToAssociate->pDesc, sizeof(association.data.Size()));
                        break;
                    }
                    case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
                        auto referencedContained = StateSubObjectWriter::Read<D3D12_LOCAL_ROOT_SIGNATURE>(*contained.pSubobjectToAssociate);
                        referencedContained.pLocalRootSignature->AddRef();

                        auto signatureState = GetState(referencedContained.pLocalRootSignature);

                        ASSERT(!_export->localSignature || _export->localSignature == signatureState, "Unexpected signature assignment");
                        _export->localSignature = signatureState;
                        break;
                    }
                }
            }
            
            state->writer.SubObjectAssociation(contained.pExports, contained.NumExports, cache.subObjects.at(contained.pSubobjectToAssociate).index);
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
            // TODO[rt]: Implement this
            ASSERT(false, "Handle it!");
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
            auto object = StateSubObjectWriter::Read<D3D12_EXISTING_COLLECTION_DESC>(*subObject);

            // Get the collection
            auto collectionTable = GetTable(object.pExistingCollection);

            // Setup export map
            std::map<std::wstring, LPCWSTR> exports;
            if (object.NumExports) {
                for (uint32_t exportIndex = 0; exportIndex < object.NumExports; exportIndex++) {
                    const D3D12_EXPORT_DESC& desc = object.pExports[exportIndex];
                    exports[desc.ExportToRename] = desc.Name;
                }
            }

            // Inherit from existing collection
            for (StateShaderSubObject& collectionSubObject: collectionTable.state->shaderSubObjects) {
                StateShaderSubObject filteredSubObject {
                    .shader = collectionSubObject.shader
                };

                // Populate lookups
                for (const StateShaderSubObjectExport& _export : collectionSubObject.functionExports) {
                    auto it = exports.find(_export.name);
                    if (!exports.empty() && it == exports.end()) {
                        continue;
                    }

                    // Copy function export
                    auto& filteredExport = filteredSubObject.functionExports.emplace_back(_export);
                    if (it != exports.end()) {
                        filteredExport.name = it->second;
                    }

                    // Add sub-object lookup (next written object)
                    state->subObjectMap[filteredExport.name] = StateSubObjectIndex {
                        .type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
                        .index = static_cast<uint32_t>(state->shaderSubObjects.size())
                    };

                    // Identifiable?
                    if (filteredExport.dxbc.type == DXBCType::Function) {
                        switch (filteredExport.dxbc.function.kind) {
                            default:
                                break;
                            case DXBCRuntimeDataShaderKind::RayGeneration:
                            case DXBCRuntimeDataShaderKind::Miss:
                            case DXBCRuntimeDataShaderKind::Callable:
                                SafeAddStateObjectIdentifier(state, filteredExport.name);
                                break;
                        }
                    }
                }

                // Any exported functions at all?
                if (!filteredSubObject.functionExports.empty()) {
                    state->shaderSubObjects.push_back(collectionSubObject);

                    // Keep linear set for instrumentation purposes
                    AddUniqueStateObjectShader(state, collectionSubObject.shader);
                
                    // Keep shaders alive
                    collectionSubObject.shader->AddUser();
                }
            }

            // Inherit hit groups from existing collection
            for (D3D12_HIT_GROUP_DESC hitGroup : collectionTable.state->hitGroupSubobjects) {
                auto it = exports.find(hitGroup.HitGroupExport);
                if (!exports.empty() && it == exports.end()) {
                    continue;
                }

                // Copy hit group
                D3D12_HIT_GROUP_DESC filteredHitGroup = hitGroup;
                if (it != exports.end()) {
                    filteredHitGroup.HitGroupExport = it->second;
                }

                // Always identifiable
                SafeAddStateObjectIdentifier(state, filteredHitGroup.HitGroupExport);

                // Add local hit group
                state->hitGroupSubobjects.push_back(filteredHitGroup);

                // Add hit group lookup
                state->subObjectMap[filteredHitGroup.HitGroupExport] = StateSubObjectIndex {
                    .type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,
                    .index = static_cast<uint32_t>(state->hitGroupSubobjects.size()) - 1
                };
            }

            // Write new object
            state->writer.DeepAdd(subObject->Type, D3D12_EXISTING_COLLECTION_DESC {
                .pExistingCollection = collectionTable.next,
                .NumExports = object.NumExports,
                .pExports = object.pExports
            });

            // Association lookup
            cache.subObjects[subObject].index = static_cast<uint32_t>(state->writer.SubObjectCount()) - 1;
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY: {
            auto object = StateSubObjectWriter::Read<D3D12_DXIL_LIBRARY_DESC>(*subObject);

            // We need to know the export names and shader kinds, so scan the DXBC for it
            TrivialStackVector<DXBCExport, 4u> dxbcExports;
            ScanDXBCShaderExports(object.DXILLibrary.pShaderBytecode, object.DXILLibrary.BytecodeLength, dxbcExports);

            // Get all exports, if none supplied, assume the dxbc exports
            // TODO[rt]: The constant wide/single swaps are a killer!
            std::vector<StateObjectExportEntry> exports;
            if (object.NumExports) {
                for (uint32_t exportIndex = 0; exportIndex < object.NumExports; exportIndex++) {
                    D3D12_EXPORT_DESC _export = object.pExports[exportIndex];
                    exports.emplace_back(StateObjectExportEntry {
                        .exportName = _export.ExportToRename ? _export.ExportToRename : _export.Name,
                        .name = _export.Name
                    });
                }
            } else {
                for (const DXBCExport& dxbcExport : dxbcExports) {
                    std::wstring unmangledWide(dxbcExport.unmangledName, dxbcExport.unmangledName + std::strlen(dxbcExport.unmangledName));
                    exports.emplace_back(StateObjectExportEntry {
                        .exportName = unmangledWide,
                        .name = unmangledWide
                    });
                }
            }

            // First, remove all replaced exports
            for (uint64_t exportIndex = 0; exportIndex < exports.size(); exportIndex++) {
                RemoveStateObjectExport(state, exports[exportIndex].name.c_str());
            }

            // Create new subobject
            StateShaderSubObject& stateSubObject = state->shaderSubObjects.emplace_back();
            stateSubObject.shader = GetOrCreateShaderState(table.state, object.DXILLibrary);
            stateSubObject.functionExports.reserve(object.NumExports);

            // All exports that will be passed down the writer
            TrivialStackVector<D3D12_EXPORT_DESC, 4u> writeExports;

            // All inline exports, handled after
            TrivialStackVector<StateObjectInlineExport, 4u> inlineExports;

            // Handle all exports
            for (uint64_t exportIndex = 0; exportIndex < exports.size(); exportIndex++) {
                const StateObjectExportEntry& exportEntry = exports[exportIndex];

                // CStrings
                LPCWSTR exportNameWide = exportEntry.exportName.c_str();
                LPCWSTR nameWide       = exportEntry.name.c_str();

                // Try to find the DXBC eqv.
                auto it = std::ranges::find_if(dxbcExports, [&](const DXBCExport& entry) { return std::wcac_equals(exportNameWide, entry.unmangledName); });
                ASSERT(it != dxbcExports.end(), "Associated export must exist in the DXBC");

                // It has to exist, anything but that is a bug
                DXBCExport dxbc = *it;

                // Function or sub-object, for now
                if (dxbc.type == DXBCType::Function) {
                    // Callable?
                    switch (dxbc.function.kind) {
                        default:
                            break;
                        case DXBCRuntimeDataShaderKind::RayGeneration:
                        case DXBCRuntimeDataShaderKind::Miss:
                        case DXBCRuntimeDataShaderKind::Callable:
                            SafeAddStateObjectIdentifier(state, nameWide);
                            break;
                    }

                    // Add to sub object exports
                    StateShaderSubObjectExport _export;
                    _export.name = nameWide;
                    _export.dxbc = dxbc;
                    stateSubObject.functionExports.push_back(_export);

                    // Keep this export in the writer
                    if (object.NumExports) {
                        writeExports.Add(object.pExports[exportIndex]);
                    } else {
                        writeExports.Add(D3D12_EXPORT_DESC {
                            .Name = state->writer.Embed<wchar_t>(nameWide, static_cast<uint32_t>(sizeof(wchar_t) * (std::wcslen(nameWide) + 1))),
                            .ExportToRename = state->writer.Embed<wchar_t>(exportNameWide, static_cast<uint32_t>(sizeof(wchar_t) * (std::wcslen(exportNameWide) + 1)))
                        });
                    }

                    // Name based lookup
                    state->subObjectMap[nameWide] = StateSubObjectIndex {
                        .type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,
                        .index = static_cast<uint32_t>(state->shaderSubObjects.size()) - 1
                    };
                } else {
                    // Handle inline sub-objects after writing the exports
                    inlineExports.Add(StateObjectInlineExport {
                        .name = nameWide,
                        .subObject = dxbc.subObject
                    });
                }
            }

            // Keep linear set for instrumentation purposes
            // Note: This holds the reference, not the sub-object
            AddUniqueStateObjectShader(state, stateSubObject.shader);

            // Embed all the exports
            auto *embeddedExports = state->writer.Alloc<D3D12_EXPORT_DESC>(static_cast<uint32_t>(sizeof(D3D12_EXPORT_DESC) * writeExports.Size()));
            for (uint64_t exportIndex = 0; exportIndex < writeExports.Size(); exportIndex++) {
                embeddedExports[exportIndex] = writeExports[exportIndex];
            }

            // Write object
            state->writer.Add(subObject->Type, D3D12_DXIL_LIBRARY_DESC {
                .DXILLibrary = object.DXILLibrary,
                .NumExports = static_cast<UINT>(writeExports.Size()),
                .pExports = embeddedExports
            });

            // Association lookup
            cache.subObjects[subObject].index = static_cast<uint32_t>(state->writer.SubObjectCount()) - 1;

            // Finally, handle all the inlined exports
            if (inlineExports.Size()) {
                CreateStateObjectInlineSubStream(table, state, inlineExports);
            }
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP: {
            auto object = StateSubObjectWriter::Read<D3D12_HIT_GROUP_DESC>(*subObject);

            // Copy all contents
            auto deepCopy = state->writer.DeepAdd(subObject->Type, object);

            // If already exists, remove the old reference
            RemoveStateObjectExport(state, object.HitGroupExport);
            
            // Keep deep copy around
            SafeAddStateObjectIdentifier(state, deepCopy->HitGroupExport);
            state->hitGroupSubobjects.push_back(*deepCopy);

            // Add hit group lookup
            state->subObjectMap[object.HitGroupExport] = StateSubObjectIndex {
                .type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,
                .index = static_cast<uint32_t>(state->hitGroupSubobjects.size()) - 1
            };

            // Association lookup
            cache.subObjects[subObject].index = static_cast<uint32_t>(state->writer.SubObjectCount()) - 1;
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE: {
            auto object = StateSubObjectWriter::Read<D3D12_GLOBAL_ROOT_SIGNATURE>(*subObject);

            // Keep the signature alive
            object.pGlobalRootSignature->AddRef();

            // Unwrap signature to the instrumented root signature
            auto rootSignature = GetTable(object.pGlobalRootSignature);
            state->signature = rootSignature.state;

            state->writer.Add(subObject->Type, D3D12_GLOBAL_ROOT_SIGNATURE {
                .pGlobalRootSignature = rootSignature.next
            });

            // Association lookup
            cache.subObjects[subObject].index = static_cast<uint32_t>(state->writer.SubObjectCount()) - 1;
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
            auto object = StateSubObjectWriter::Read<D3D12_LOCAL_ROOT_SIGNATURE>(*subObject);

            // Unwrap signature to the *native* signature
            // Instrumented local root signature association happens later after record patching
            state->writer.Add(subObject->Type, D3D12_LOCAL_ROOT_SIGNATURE {
                .pLocalRootSignature = GetState(object.pLocalRootSignature)->nativeObject
            });

            // Association lookup
            cache.subObjects[subObject].index = static_cast<uint32_t>(state->writer.SubObjectCount()) - 1;
            break;
        }
    }
}

static bool SubObjectExportHasAssociation(const StateShaderSubObjectExport& _export, D3D12_STATE_SUBOBJECT_TYPE type) {
    for (const StateSubObjectAssociation& association : _export.associations) {
        if (association.type == type) {
            return true;
        }
    }

    return false;
}

void CreateDefaultAssociation(StateObjectState* state, const D3D12_STATE_SUBOBJECT* subObject, StateObjectCache& cache) {
    // Find entry, may not exist
    auto&& entryIt = cache.subObjects.find(subObject);
    if (entryIt == cache.subObjects.end()) {
        return;
    }

    // If associated, ignore
    StateObjectEntry& entry = entryIt->second;
    if (entry.associated) {
        return;
    }

    // This sub-object hasn't been associated, check if it's a candidate for default association
    switch (subObject->Type) {
        default: {
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG: {            
            // Associate all shaders with the shader config, unless already specified
            for (StateShaderSubObject& shader : state->shaderSubObjects) {
                for (StateShaderSubObjectExport& _export : shader.functionExports) {
                    if (SubObjectExportHasAssociation(_export, subObject->Type)) {
                        continue;
                    }

                    // Not part of the export, add association
                    StateSubObjectAssociation &association = _export.associations.emplace_back();
                    association.type = subObject->Type;
                    association.data.Resize(StateSubObjectWriter::GetSize(association.type));
                    std::memcpy(association.data.Data(), subObject->pDesc, sizeof(association.data.Size()));
                } 
            }
            
            break;
        }
        case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
            auto referencedContained = StateSubObjectWriter::Read<D3D12_LOCAL_ROOT_SIGNATURE>(*subObject);
            referencedContained.pLocalRootSignature->AddRef();

            // Get state
            auto signatureState = GetState(referencedContained.pLocalRootSignature);

            // Associate all shaders with the local signature, unless already specified
            for (StateShaderSubObject& shader : state->shaderSubObjects) {
                for (StateShaderSubObjectExport& _export : shader.functionExports) {
                    if (!_export.localSignature) {
                        _export.localSignature = signatureState;
                    }
                } 
            } 
        }
    }
}

void CreateStateSubObjects(const DeviceTable& table, StateObjectState* state, const D3D12_STATE_OBJECT_DESC* pDesc) {
    // Local cache for associations
    StateObjectCache cache;

    // Unwrap objects
    for (uint32_t i = 0; i < pDesc->NumSubobjects; i++) {
        CreateStateSubObject(table, state, &pDesc->pSubobjects[i], cache);
    }

    // Create default associations
    for (uint32_t i = 0; i < pDesc->NumSubobjects; i++) {
        CreateDefaultAssociation(state, &pDesc->pSubobjects[i], cache);
    }
}

static void InheritStateObject(StateObjectState* stateObject, StateObjectState* source) {
    // We don't want to keep a reference to the source state object,
    // since it may keep growing throughout the application.

    // Inherit pipeline state
    stateObject->shaders = source->shaders;
    stateObject->signature = source->signature;

    // Inherit state-object state, keeping the index map as-is is fine
    stateObject->shaderSubObjects   = source->shaderSubObjects;
    stateObject->hitGroupSubobjects = source->hitGroupSubobjects;
    stateObject->identifierExports  = source->identifierExports;
    stateObject->subObjectMap       = source->subObjectMap;

    // Add shader reference counts
    for (ShaderState* shader : stateObject->shaders) {
        shader->AddUser();
    }

    // Deep copy all sub-object writes
    D3D12_STATE_OBJECT_DESC desc = source->writer.GetUnresolvedDesc();
    for (uint32_t i = 0; i < desc.NumSubobjects; i++) {
        D3D12_STATE_SUBOBJECT subObject = desc.pSubobjects[i];
        
        // Either directly handled or inherited
        if (subObject.Type == D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION ||
            subObject.Type == D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION ||
            subObject.Type == D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION ||
            subObject.Type == D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP ||
            subObject.Type == D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY ||
            subObject.Type == D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE) {
            continue;
        }
        
        stateObject->writer.DeepAdd(subObject.Type, subObject.pDesc);
    }
}

static HRESULT CreateOrAddToStateObject(ID3D12Device2* device, const D3D12_STATE_OBJECT_DESC* pDesc, ID3D12StateObject* existingStateObject, const IID& riid, void** ppStateObject) {
    auto table = GetTable(device);

    // Tag all allocations
    auto allocators = table.state->allocators.Tag(kAllocStateStateObject);

    // Create state
    auto* state = new (allocators) StateObjectState(allocators);
    state->parent = device;
    state->type = PipelineType::StateObject;
    state->stateObjectType = pDesc->Type;
    
    // Reserve on the actual sub-object count
    state->writer.Reserve(pDesc->NumSubobjects);

    // If existing, inherit
    if (existingStateObject) {
        auto existingTable = GetTable(existingStateObject);
        InheritStateObject(state, existingTable.state);
    }

    // If inherited, we don't create the description from the first sub-object
    // Instrumentation will re-create everything, but we keep the performance as is here
    uint32_t inheritedSubobjects = static_cast<uint32_t>(state->writer.SubObjectCount());

    // Create the sub-objects, handles inline as well
    CreateStateSubObjects(table, state, pDesc);

    // Create description
    D3D12_STATE_OBJECT_DESC desc = state->writer.GetDesc(pDesc->Type, inheritedSubobjects);

    // Object
    ID3D12StateObject* stateObject{nullptr};

    // Pass down callchain(s)
    if (existingStateObject) {
        HRESULT hr = table.next->AddToStateObject(&desc, Next(existingStateObject), __uuidof(ID3D12StateObject), reinterpret_cast<void**>(&stateObject));
        if (FAILED(hr)) {
            return hr;
        }
    } else {
        HRESULT hr = table.next->CreateStateObject(&desc, __uuidof(ID3D12StateObject), reinterpret_cast<void**>(&stateObject));
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Keep native around
    state->object = stateObject;
    
    // External users
    device->AddRef();
    state->AddUser();

    // Create identifier table for patching
    CreateStateObjectIdentifierTable(table, state);

    // Create detours
    stateObject = CreateDetour(state->allocators, stateObject, state);

    // Query to external object if requested
    if (ppStateObject) {
        HRESULT hr = stateObject->QueryInterface(riid, ppStateObject);
        if (FAILED(hr)) {
            return hr;
        }

        // Inform the controller
        table.state->instrumentationController->CreatePipelineAndAdd(state);
    }

    // Cleanup
    stateObject->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12DeviceCreateStateObject(ID3D12Device2* device, const D3D12_STATE_OBJECT_DESC* pDesc, const IID& riid, void** ppStateObject) {
    return CreateOrAddToStateObject(device, pDesc, nullptr, riid, ppStateObject);
}

HRESULT WINAPI HookID3D12DeviceAddToStateObject(ID3D12Device2* device, const D3D12_STATE_OBJECT_DESC* pAddition, ID3D12StateObject* pStateObjectToGrowFrom, const IID& riid, void** ppNewStateObject) {
    return CreateOrAddToStateObject(device, pAddition, pStateObjectToGrowFrom, riid, ppNewStateObject);
}

HRESULT WINAPI HookID3D12StateObjectGetDevice(ID3D12StateObject* _this, REFIID riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevice);
}
