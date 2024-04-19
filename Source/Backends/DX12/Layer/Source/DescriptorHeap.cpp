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

#include <Backends/DX12/DescriptorHeap.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/DescriptorHeapState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Export/ShaderExportStreamer.h>
#include <Backends/DX12/Export/ShaderExportFixedTwoSidedDescriptorAllocator.h>
#include <Backends/DX12/Resource/PhysicalResourceMappingTable.h>
#include <Backends/DX12/Translation.h>

// Backend
#include <Backend/IL/ResourceTokenType.h>

HRESULT WINAPI HookID3D12DeviceCreateDescriptorHeap(ID3D12Device *device, const D3D12_DESCRIPTOR_HEAP_DESC *desc, REFIID riid, void **pHeap) {
    auto table = GetTable(device);

    // Create state
    auto* state = new (table.state->allocators, kAllocStateDescriptorHeap) DescriptorHeapState();
    state->allocators = table.state->allocators;
    state->parent = device;
    state->type = desc->Type;
    state->flags = desc->Flags;
    state->physicalDescriptorCount = desc->NumDescriptors;
    state->virtualDescriptorCount = desc->NumDescriptors;
    state->exhausted = false;

    // Keep reference
    device->AddRef();

    // Object
    ID3D12DescriptorHeap* heap{nullptr};

    // Heap of interest?
    if (desc->Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
        // Get desired bound
        uint32_t bound = ShaderExportFixedTwoSidedDescriptorAllocator::GetDescriptorBound(table.state->exportHost.GetUnsafe());

        // There is little to no insight for the internal driver limits, so, attempt various sizes
        for (uint32_t divisor = 0; divisor < 4; divisor++) {
            // Copy description
            D3D12_DESCRIPTOR_HEAP_DESC expandedHeap = *desc;
            expandedHeap.NumDescriptors += bound;

            // Pass down callchain
            HRESULT hr = table.next->CreateDescriptorHeap(&expandedHeap, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&heap));
            if (SUCCEEDED(hr)) {
                break;
            }

            // Attempt next count
            bound /= 2;

            // Invalidate
            heap = nullptr;
        }

        // Succeeded?
        if (heap) {
            // Set count
            state->physicalDescriptorCount = desc->NumDescriptors + bound;

            // Create unique allocator
            state->allocator = table.state->exportStreamer->AllocateTwoSidedAllocator(heap, desc->NumDescriptors, bound);
        }
    }

    // If exhausted, fall back to user requirements
    if (!heap) {
        state->exhausted = true;

        // Pass down callchain
        HRESULT hr = table.bottom->next_CreateDescriptorHeap(table.next, desc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&heap));
        if (FAILED(hr)) {
            return hr;
        }
    }
            
    // GPU base if heap supports
    if (desc->Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
        state->gpuDescriptorBase = heap->GetGPUDescriptorHandleForHeapStart();
    }

    // Set base
    state->cpuDescriptorBase = heap->GetCPUDescriptorHandleForHeapStart();
    state->stride = device->GetDescriptorHandleIncrementSize(desc->Type);
    
    // Create PRMT
    state->prmTable = new (table.state->allocators, kAllocPRMT) PhysicalResourceMappingTable(table.state->allocators.Tag(kAllocPRMT), table.state->deviceAllocator);

    // Initialize table with count
    state->prmTable->Install(state->type, state->physicalDescriptorCount);

    // Register heap in shared table
    table.state->cpuHeapTable.Add(state->type, state, state->cpuDescriptorBase.ptr, state->physicalDescriptorCount, state->stride);

    // GPU visibility optional
    if (desc->Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
        table.state->gpuHeapTable.Add(state->type, state, state->gpuDescriptorBase.ptr, state->physicalDescriptorCount, state->stride);
    }

    // Create detours
    heap = CreateDetour(state->allocators, heap, state);

    // Query to external object if requested
    if (pHeap) {
        HRESULT hr = heap->QueryInterface(riid, pHeap);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    heap->Release();

    // OK
    return S_OK;
}

void WINAPI HookID3D12DescriptorHeapGetCPUDescriptorHandleForHeapStart(ID3D12DescriptorHeap *heap, D3D12_CPU_DESCRIPTOR_HANDLE* out) {
    auto table = GetTable(heap);

    D3D12_CPU_DESCRIPTOR_HANDLE reg;
    table.bottom->next_GetCPUDescriptorHandleForHeapStart(table.next, &reg);

    // Advance base by heap prefix
    if (table.state->allocator) {
        reg.ptr += table.state->allocator->GetPrefixOffset();
    }

    *out = reg;
}

void WINAPI HookID3D12DescriptorHeapGetGPUDescriptorHandleForHeapStart(ID3D12DescriptorHeap *heap, D3D12_GPU_DESCRIPTOR_HANDLE* out) {
    auto table = GetTable(heap);

    D3D12_GPU_DESCRIPTOR_HANDLE reg;
    table.bottom->next_GetGPUDescriptorHandleForHeapStart(table.next, &reg);

    // Advance base by heap prefix
    if (table.state->allocator && table.state->flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
        reg.ptr += table.state->allocator->GetPrefixOffset();
    }

    *out = reg;
}

static VirtualResourceMapping GetNullResourceMapping(D3D12_SRV_DIMENSION dimension) {
    // Handle by dimension
    switch (dimension) {
        default: {
            ASSERT(false, "Unsupported value");
            return {};
        }
        case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE:
        case D3D12_SRV_DIMENSION_BUFFER:{
            return VirtualResourceMapping {
                ResourceToken {
                    .puid = IL::kResourceTokenPUIDReservedNullBuffer,
                    .type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Buffer)
                }
            };
        }
        case D3D12_SRV_DIMENSION_TEXTURE1D:
        case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
        case D3D12_SRV_DIMENSION_TEXTURE2D:
        case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
        case D3D12_SRV_DIMENSION_TEXTURE2DMS:
        case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
        case D3D12_SRV_DIMENSION_TEXTURE3D:
        case D3D12_SRV_DIMENSION_TEXTURECUBE:
        case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY: {
            return VirtualResourceMapping {
                ResourceToken {
                    .puid = IL::kResourceTokenPUIDReservedNullTexture,
                    .type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Texture)
                }
            };
        }
    }
}

static VirtualResourceMapping GetNullResourceMapping(D3D12_RTV_DIMENSION dimension) {
    // Handle by dimension
    switch (dimension) {
        default: {
            ASSERT(false, "Unsupported value");
            return {};
        }
        case D3D12_RTV_DIMENSION_BUFFER:{
            return VirtualResourceMapping {
                ResourceToken {
                    .puid = IL::kResourceTokenPUIDReservedNullBuffer,
                    .type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Buffer)
                }
            };
        }
        case D3D12_RTV_DIMENSION_TEXTURE1D:
        case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
        case D3D12_RTV_DIMENSION_TEXTURE2D:
        case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
        case D3D12_RTV_DIMENSION_TEXTURE2DMS:
        case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
        case D3D12_RTV_DIMENSION_TEXTURE3D: {
            return VirtualResourceMapping {
                ResourceToken {
                    .puid = IL::kResourceTokenPUIDReservedNullTexture,
                    .type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Texture)
                }
            };
        }
    }
}

static VirtualResourceMapping GetNullResourceMapping(D3D12_UAV_DIMENSION dimension) {
    // Handle by dimension
    switch (dimension) {
        default: {
            ASSERT(false, "Unsupported value");
            return {};
        }
        case D3D12_UAV_DIMENSION_BUFFER:{
            return VirtualResourceMapping {
                ResourceToken {
                    .puid = IL::kResourceTokenPUIDReservedNullBuffer,
                    .type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Buffer)
                }
            };
        }
        case D3D12_UAV_DIMENSION_TEXTURE1D:
        case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
        case D3D12_UAV_DIMENSION_TEXTURE2D:
        case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
        case D3D12_UAV_DIMENSION_TEXTURE2DMS:
        case D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY:
        case D3D12_UAV_DIMENSION_TEXTURE3D: {
            return VirtualResourceMapping {
                ResourceToken {
                    .puid = IL::kResourceTokenPUIDReservedNullTexture,
                    .type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Texture)
                }
            };
        }
    }
}

static VirtualResourceMapping GetNullResourceMapping(D3D12_DSV_DIMENSION dimension) {
    // Handle by dimension
    switch (dimension) {
        default: {
            ASSERT(false, "Unsupported value");
            return {};
        }
        case D3D12_DSV_DIMENSION_TEXTURE1D:
        case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
        case D3D12_DSV_DIMENSION_TEXTURE2D:
        case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
        case D3D12_DSV_DIMENSION_TEXTURE2DMS:
        case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY: {
            return VirtualResourceMapping {
                ResourceToken {
                    .puid = IL::kResourceTokenPUIDReservedNullTexture,
                    .type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Texture)
                }
            };
        }
    }
}

/// Handle all defaults mappings in a virtual mapping
/// \param state source state
/// \param mapping destination mapping
void HandleMappingDefaults(ResourceState* state, VirtualResourceMapping& mapping) {
    // Automatic mip count?
    if (mapping.token.viewMipCount == -1) {
        mapping.token.viewMipCount = state->desc.MipLevels - mapping.token.viewBaseMip;
    }

    // Automatic slice count?
    if (mapping.token.viewSliceCount == -1) {
        mapping.token.viewSliceCount = state->desc.DepthOrArraySize - mapping.token.viewBaseSlice;
    }

    // Adjust new format
    // todo[init]: complete!
#if 0
    if (pDesc->Format == DXGI_FORMAT_UNKNOWN) {
        ASSERT(pDesc->ViewDimension == D3D12_SRV_DIMENSION_BUFFER, "Unexpected view dimension");
        mapping.formatSize = pDesc->Buffer.StructureByteStride;
    } else {
        mapping.formatId = static_cast<uint32_t>(Translate(pDesc->Format));
        mapping.formatSize = GetFormatByteSize(pDesc->Format);

        // Report the typed size
        if (resource.state->desc.Format == DXGI_FORMAT_UNKNOWN) {
            mapping.width /= mapping.formatSize;
        } else {
            ASSERT(mapping.formatSize == resource.state->virtualMapping.formatSize, "Unexpected format size");
        }
    }
#endif
}

void WINAPI HookID3D12DeviceCreateShaderResourceView(ID3D12Device* _this, ID3D12Resource* pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
    auto table = GetTable(_this);
    auto resource = GetTable(pResource);

    // Associated heap?
    if (DescriptorHeapState* heap = table.state->cpuHeapTable.Find(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, DestDescriptor.ptr)) {
        const uint64_t offset = DestDescriptor.ptr - heap->cpuDescriptorBase.ptr;
        ASSERT(offset % heap->stride == 0, "Invalid heap offset");

        // Table wise offset
        const uint32_t tableOffset = static_cast<uint32_t>(offset / heap->stride);
        
        // Null descriptors are handled separately
        if (pResource) {
            VirtualResourceMapping mapping = resource.state->virtualMapping;

            if (pDesc) {
                switch (pDesc->ViewDimension) {
                    case D3D12_SRV_DIMENSION_UNKNOWN:
                    case D3D12_SRV_DIMENSION_TEXTURE2DMS:
                    case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE:
                        break;
                    case D3D12_SRV_DIMENSION_BUFFER:
                        mapping.token.viewBaseWidth = static_cast<uint32_t>(pDesc->Buffer.FirstElement);
                        mapping.token.viewWidth = static_cast<uint32_t>(pDesc->Buffer.NumElements);
                        break;
                    case D3D12_SRV_DIMENSION_TEXTURE1D:
                        mapping.token.viewBaseMip = pDesc->Texture1D.MostDetailedMip;
                        mapping.token.viewMipCount = pDesc->Texture1D.MipLevels;
                        break;
                    case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
                        mapping.token.viewBaseMip = pDesc->Texture1DArray.MostDetailedMip;
                        mapping.token.viewMipCount = pDesc->Texture1DArray.MipLevels;
                        mapping.token.viewBaseSlice = pDesc->Texture1DArray.FirstArraySlice;
                        mapping.token.viewSliceCount = pDesc->Texture1DArray.ArraySize;
                        break;
                    case D3D12_SRV_DIMENSION_TEXTURE2D:
                        mapping.token.viewBaseMip = pDesc->Texture2D.MostDetailedMip;
                        mapping.token.viewMipCount = pDesc->Texture2D.MipLevels;
                        break;
                    case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
                        mapping.token.viewBaseMip = pDesc->Texture2DArray.MostDetailedMip;
                        mapping.token.viewMipCount = pDesc->Texture2DArray.MipLevels;
                        mapping.token.viewBaseSlice = pDesc->Texture2DArray.FirstArraySlice;
                        mapping.token.viewSliceCount = pDesc->Texture2DArray.ArraySize;
                        break;
                    case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
                        mapping.token.viewBaseSlice = pDesc->Texture2DMSArray.FirstArraySlice;
                        mapping.token.viewSliceCount = pDesc->Texture2DMSArray.ArraySize;
                        break;
                    case D3D12_SRV_DIMENSION_TEXTURE3D:
                        mapping.token.viewBaseMip = pDesc->Texture3D.MostDetailedMip;
                        mapping.token.viewMipCount = pDesc->Texture3D.MipLevels;
                        break;
                    case D3D12_SRV_DIMENSION_TEXTURECUBE:
                        mapping.token.viewBaseMip = pDesc->TextureCube.MostDetailedMip;
                        mapping.token.viewMipCount = pDesc->TextureCube.MipLevels;
                        break;
                    case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
                        mapping.token.viewBaseMip = pDesc->TextureCubeArray.MostDetailedMip;
                        mapping.token.viewMipCount = pDesc->TextureCubeArray.MipLevels;
                        mapping.token.viewBaseSlice = pDesc->TextureCubeArray.First2DArrayFace;
                        mapping.token.viewSliceCount = -1;
                        break;
                }

                // Compute the defaults
                HandleMappingDefaults(resource.state, mapping);
            }
            
            heap->prmTable->WriteMapping(tableOffset, resource.state, mapping);
        } else {
            heap->prmTable->WriteMapping(tableOffset, nullptr, GetNullResourceMapping(pDesc->ViewDimension));
        }
    } else {
        ASSERT(false, "Failed to associate descriptor handle to heap");
    }

    // Pass down callchain
    table.next->CreateShaderResourceView(resource.next, pDesc, DestDescriptor);
}

void WINAPI HookID3D12DeviceCreateUnorderedAccessView(ID3D12Device* _this, ID3D12Resource* pResource, ID3D12Resource* pCounterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
    auto table = GetTable(_this);
    auto resource = GetTable(pResource);

    // Associated heap?
    if (DescriptorHeapState* heap = table.state->cpuHeapTable.Find(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, DestDescriptor.ptr)) {
        const uint64_t offset = DestDescriptor.ptr - heap->cpuDescriptorBase.ptr;
        ASSERT(offset % heap->stride == 0, "Invalid heap offset");

        // Table wise offset
        const uint32_t tableOffset = static_cast<uint32_t>(offset / heap->stride);
        
        // Null descriptors are handled separately
        if (pResource) {
            VirtualResourceMapping mapping = resource.state->virtualMapping;

            if (pDesc) {
                switch (pDesc->ViewDimension) {
                    case D3D12_UAV_DIMENSION_UNKNOWN:
                    case D3D12_UAV_DIMENSION_TEXTURE2DMS:
                        break;
                    case D3D12_SRV_DIMENSION_BUFFER:
                        mapping.token.viewBaseWidth = static_cast<uint32_t>(pDesc->Buffer.FirstElement);
                        mapping.token.viewWidth = static_cast<uint32_t>(pDesc->Buffer.NumElements);
                        break;
                    case D3D12_UAV_DIMENSION_TEXTURE1D:
                        mapping.token.viewBaseMip = pDesc->Texture1D.MipSlice;
                        mapping.token.viewMipCount = -1;
                        break;
                    case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
                        mapping.token.viewBaseMip = pDesc->Texture1DArray.MipSlice;
                        mapping.token.viewMipCount = -1;
                        mapping.token.viewBaseSlice = pDesc->Texture1DArray.FirstArraySlice;
                        mapping.token.viewSliceCount = pDesc->Texture1DArray.ArraySize;
                        break;
                    case D3D12_UAV_DIMENSION_TEXTURE2D:
                        mapping.token.viewBaseMip = pDesc->Texture2D.MipSlice;
                        mapping.token.viewMipCount = -1;
                        break;
                    case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
                        mapping.token.viewBaseMip = pDesc->Texture2DArray.MipSlice;
                        mapping.token.viewMipCount = -1;
                        mapping.token.viewBaseSlice = pDesc->Texture2DArray.FirstArraySlice;
                        mapping.token.viewSliceCount = pDesc->Texture2DArray.ArraySize;
                        break;
                    case D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY:
                        mapping.token.viewBaseSlice = pDesc->Texture2DMSArray.FirstArraySlice;
                        mapping.token.viewSliceCount = pDesc->Texture2DMSArray.ArraySize;
                        break;
                    case D3D12_UAV_DIMENSION_TEXTURE3D:
                        mapping.token.viewBaseMip = pDesc->Texture3D.MipSlice;
                        mapping.token.viewMipCount = -1;
                        break;
                }

                // Compute the defaults
                HandleMappingDefaults(resource.state, mapping);
            }
            
            heap->prmTable->WriteMapping(tableOffset, resource.state, mapping);
        } else {
            heap->prmTable->WriteMapping(tableOffset, nullptr, GetNullResourceMapping(pDesc->ViewDimension));
        }
    } else {
        ASSERT(false, "Failed to associate descriptor handle to heap");
    }

    // Pass down callchain
    table.next->CreateUnorderedAccessView(resource.next, Next(pCounterResource), pDesc, DestDescriptor);
}

void WINAPI HookID3D12DeviceCreateRenderTargetView(ID3D12Device* _this, ID3D12Resource* pResource, const D3D12_RENDER_TARGET_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
    auto table = GetTable(_this);
    auto resource = GetTable(pResource);

    // Associated heap?
    if (DescriptorHeapState* heap = table.state->cpuHeapTable.Find(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, DestDescriptor.ptr)) {
        uint64_t offset = DestDescriptor.ptr - heap->cpuDescriptorBase.ptr;
        ASSERT(offset % heap->stride == 0, "Invalid heap offset");

        // Table wise offset
        const uint32_t tableOffset = static_cast<uint32_t>(offset / heap->stride);

        // Null descriptors are handled separately
        if (pResource) {
            VirtualResourceMapping mapping = resource.state->virtualMapping;
            
            if (pDesc) {
                switch (pDesc->ViewDimension) {
                    case D3D12_RTV_DIMENSION_UNKNOWN:
                    case D3D12_RTV_DIMENSION_TEXTURE2DMS:
                        break;
                    case D3D12_RTV_DIMENSION_BUFFER:
                        mapping.token.viewBaseWidth = static_cast<uint32_t>(pDesc->Buffer.FirstElement);
                        mapping.token.viewWidth = static_cast<uint32_t>(pDesc->Buffer.NumElements);
                        break;
                    case D3D12_RTV_DIMENSION_TEXTURE1D:
                        mapping.token.viewBaseMip = pDesc->Texture1D.MipSlice;
                        mapping.token.viewMipCount = -1;
                        break;
                    case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
                        mapping.token.viewBaseMip = pDesc->Texture1DArray.MipSlice;
                        mapping.token.viewMipCount = -1;
                        mapping.token.viewBaseSlice = pDesc->Texture1DArray.FirstArraySlice;
                        mapping.token.viewSliceCount = pDesc->Texture1DArray.ArraySize;
                        break;
                    case D3D12_RTV_DIMENSION_TEXTURE2D:
                        mapping.token.viewBaseMip = pDesc->Texture2D.MipSlice;
                        mapping.token.viewMipCount = -1;
                        break;
                    case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
                        mapping.token.viewBaseMip = pDesc->Texture2DArray.MipSlice;
                        mapping.token.viewMipCount = -1;
                        mapping.token.viewBaseSlice = pDesc->Texture2DArray.FirstArraySlice;
                        mapping.token.viewSliceCount = pDesc->Texture2DArray.ArraySize;
                        break;
                    case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
                        mapping.token.viewBaseSlice = pDesc->Texture2DMSArray.FirstArraySlice;
                        mapping.token.viewSliceCount = pDesc->Texture2DMSArray.ArraySize;
                        break;
                    case D3D12_RTV_DIMENSION_TEXTURE3D:
                        mapping.token.viewBaseMip = pDesc->Texture3D.MipSlice;
                        mapping.token.viewMipCount = -1;
                        break;
                }

                // Compute the defaults
                HandleMappingDefaults(resource.state, mapping);
            }
            
            heap->prmTable->WriteMapping(tableOffset, resource.state, mapping);
        } else {
            heap->prmTable->WriteMapping(tableOffset, nullptr, GetNullResourceMapping(pDesc->ViewDimension));
        }
    } else {
        ASSERT(false, "Failed to associate descriptor handle to heap");
    }

    // Pass down callchain
    table.next->CreateRenderTargetView(resource.next, pDesc, DestDescriptor);
}

void WINAPI HookID3D12DeviceCreateDepthStencilView(ID3D12Device* _this, ID3D12Resource* pResource, const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
    auto table = GetTable(_this);
    auto resource = GetTable(pResource);

    // Associated heap?
    if (DescriptorHeapState* heap = table.state->cpuHeapTable.Find(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DestDescriptor.ptr)) {
        uint64_t offset = DestDescriptor.ptr - heap->cpuDescriptorBase.ptr;
        ASSERT(offset % heap->stride == 0, "Invalid heap offset");

        // Table wise offset
        const uint32_t tableOffset = static_cast<uint32_t>(offset / heap->stride);

        // Null descriptors are handled separately
        if (pResource) {
            VirtualResourceMapping mapping = resource.state->virtualMapping;
            
            if (pDesc) {
                switch (pDesc->ViewDimension) {
                    case D3D12_DSV_DIMENSION_UNKNOWN:
                    case D3D12_DSV_DIMENSION_TEXTURE2DMS:
                        break;
                    case D3D12_DSV_DIMENSION_TEXTURE1D:
                        mapping.token.viewBaseMip = pDesc->Texture1D.MipSlice;
                        mapping.token.viewMipCount = -1;
                        break;
                    case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
                        mapping.token.viewBaseMip = pDesc->Texture1DArray.MipSlice;
                        mapping.token.viewMipCount = -1;
                        mapping.token.viewBaseSlice = pDesc->Texture1DArray.FirstArraySlice;
                        mapping.token.viewSliceCount = pDesc->Texture1DArray.ArraySize;
                        break;
                    case D3D12_DSV_DIMENSION_TEXTURE2D:
                        mapping.token.viewBaseMip = pDesc->Texture2D.MipSlice;
                        mapping.token.viewMipCount = -1;
                        break;
                    case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
                        mapping.token.viewBaseMip = pDesc->Texture2DArray.MipSlice;
                        mapping.token.viewMipCount = -1;
                        mapping.token.viewBaseSlice = pDesc->Texture2DArray.FirstArraySlice;
                        mapping.token.viewSliceCount = pDesc->Texture2DArray.ArraySize;
                        break;
                    case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
                        mapping.token.viewBaseSlice = pDesc->Texture2DMSArray.FirstArraySlice;
                        mapping.token.viewSliceCount = pDesc->Texture2DMSArray.ArraySize;
                        break;
                }

                // Compute the defaults
                HandleMappingDefaults(resource.state, mapping);
            }
            
            heap->prmTable->WriteMapping(tableOffset, resource.state, mapping);
        } else {
            heap->prmTable->WriteMapping(tableOffset, nullptr, GetNullResourceMapping(pDesc->ViewDimension));
        }
    } else {
        ASSERT(false, "Failed to associate descriptor handle to heap");
    }

    // Pass down callchain
    table.next->CreateDepthStencilView(resource.next, pDesc, DestDescriptor);
}

void WINAPI HookID3D12DeviceCreateSampler(ID3D12Device* _this, const D3D12_SAMPLER_DESC* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
    auto table = GetTable(_this);

    // Associated heap?
    if (DescriptorHeapState* heap = table.state->cpuHeapTable.Find(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, DestDescriptor.ptr)) {
        const uint64_t offset = DestDescriptor.ptr - heap->cpuDescriptorBase.ptr;
        ASSERT(offset % heap->stride == 0, "Invalid heap offset");

        // Table wise offset
        const uint32_t tableOffset = static_cast<uint32_t>(offset / heap->stride);

        // Write mapping
        heap->prmTable->WriteMapping(tableOffset, nullptr, VirtualResourceMapping {
            ResourceToken {
                .puid = 0,
                .type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Sampler)
            }
        });
    } else {
        ASSERT(false, "Failed to associate descriptor handle to heap");
    }

    // Pass down callchain
    table.next->CreateSampler(pDesc, DestDescriptor);
}

void WINAPI HookID3D12DeviceCreateSampler2(ID3D12Device* _this, const D3D12_SAMPLER_DESC2* pDesc, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
    auto table = GetTable(_this);

    // Associated heap?
    if (DescriptorHeapState* heap = table.state->cpuHeapTable.Find(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, DestDescriptor.ptr)) {
        const uint64_t offset = DestDescriptor.ptr - heap->cpuDescriptorBase.ptr;
        ASSERT(offset % heap->stride == 0, "Invalid heap offset");

        // Table wise offset
        const uint32_t tableOffset = static_cast<uint32_t>(offset / heap->stride);

        // Write mapping
        heap->prmTable->WriteMapping(tableOffset, nullptr, VirtualResourceMapping {
            ResourceToken {
                .puid = 0,
                .type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Sampler)
            }
        });
    } else {
        ASSERT(false, "Failed to associate descriptor handle to heap");
    }

    // Pass down callchain
    table.next->CreateSampler2(pDesc, DestDescriptor);
}

void WINAPI HookID3D12DeviceCopyDescriptors(ID3D12Device* _this, UINT NumDestDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE* pDestDescriptorRangeStarts, const UINT* pDestDescriptorRangeSizes, UINT NumSrcDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorRangeStarts, const UINT* pSrcDescriptorRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType) {
    auto table = GetTable(_this);

    // Get iteration for the heap type
    const uint32_t incrementForHeap = table.next->GetDescriptorHandleIncrementSize(DescriptorHeapsType);

    // Source iteration
    uint32_t srcRangeOffset = 0;
    uint32_t srcDescriptorOffset = 0;

    // Handle all ranges
    for (uint32_t rangeIndex = 0; rangeIndex < NumDestDescriptorRanges; rangeIndex++) {
        D3D12_CPU_DESCRIPTOR_HANDLE dst = pDestDescriptorRangeStarts[rangeIndex];

        // Get destination heap
        // Note: Heaps may change between each range, no guarantee
        const DescriptorHeapState* dstHeap = table.state->cpuHeapTable.Find(DescriptorHeapsType, dst.ptr);
        
        // Default the range size
        const uint32_t dstRangeSize = pDestDescriptorRangeSizes ? pDestDescriptorRangeSizes[rangeIndex] : 1u;

        // Handle all descriptors within the range
        for (uint32_t descriptorIndex = 0; descriptorIndex < dstRangeSize; descriptorIndex++) {
            // Deduce source handle
            const D3D12_CPU_DESCRIPTOR_HANDLE src = {pSrcDescriptorRangeStarts[srcRangeOffset].ptr + incrementForHeap * (srcDescriptorOffset++)};

            // Get source heap
            // Note: Heaps may change between each range, no guarantee
            DescriptorHeapState* srcHeap = table.state->cpuHeapTable.Find(DescriptorHeapsType, src.ptr);

            // Validation
            ASSERT(srcHeap && dstHeap, "Failed to associate descriptor handle to heap");
            
            // Valid heaps?
            if (srcHeap && dstHeap) {
                const uint32_t srcOffset = srcHeap->GetOffsetFromHeapHandle(src);
                
                // Copy the mapping
                dstHeap->prmTable->WriteMapping(
                    dstHeap->GetOffsetFromHeapHandle(dst),
                    srcHeap->prmTable->GetMappingState(srcOffset),
                    srcHeap->prmTable->GetMapping(srcOffset)
                );
            }

            // Default the source range size
            const uint32_t srcRangeSize = pSrcDescriptorRangeSizes ? pSrcDescriptorRangeSizes[srcRangeOffset] : 1u;

            // Exceeded source range? Roll!
            if (srcDescriptorOffset >= srcRangeSize) {
                srcDescriptorOffset = 0;
                srcRangeOffset++;
            }

            // Increment destination offset
            dst.ptr += incrementForHeap;
        }
    }

    // Pass down callchain
    table.next->CopyDescriptors(NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes, NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes, DescriptorHeapsType);
}

void WINAPI HookID3D12DeviceCopyDescriptorsSimple(ID3D12Device* _this, UINT NumDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart, D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart, D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType) {
    auto table = GetTable(_this);
    
    // Get iteration for the heap type
    const uint32_t incrementForHeap = table.next->GetDescriptorHandleIncrementSize(DescriptorHeapsType);

    // Get heaps
    const DescriptorHeapState* dstHeap = table.state->cpuHeapTable.Find(DescriptorHeapsType, DestDescriptorRangeStart.ptr);
    const DescriptorHeapState* srcHeap = table.state->cpuHeapTable.Find(DescriptorHeapsType, SrcDescriptorRangeStart.ptr);

    // Validation
    ASSERT(dstHeap && srcHeap, "Failed to associate descriptor handle to heap");
    
    // Valid heaps?
    if (srcHeap && dstHeap) {
        D3D12_CPU_DESCRIPTOR_HANDLE dst = DestDescriptorRangeStart;
        D3D12_CPU_DESCRIPTOR_HANDLE src = SrcDescriptorRangeStart;
        
        // Handle all ranges
        for (uint32_t descriptorIndex = 0; descriptorIndex < NumDescriptors; descriptorIndex++) {
            // Get offsets
            const uint32_t dstOffset = dstHeap->GetOffsetFromHeapHandle(dst);
            const uint32_t srcOffset = srcHeap->GetOffsetFromHeapHandle(src);
            
            // Copy the mapping
            dstHeap->prmTable->WriteMapping(
                dstOffset,
                srcHeap->prmTable->GetMappingState(srcOffset),
                srcHeap->prmTable->GetMapping(srcOffset)
            );

            // Increment destination offset
            dst.ptr += incrementForHeap;
            src.ptr += incrementForHeap;
        }
    }

    // Pass down callchain
    table.next->CopyDescriptorsSimple(NumDescriptors, DestDescriptorRangeStart, SrcDescriptorRangeStart, DescriptorHeapsType);
}

HRESULT WINAPI HookID3D12DescriptorHeapGetDevice(ID3D12DescriptorHeap*_this, const IID &riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevice);
}

DescriptorHeapState::~DescriptorHeapState() {
    auto table = GetTable(parent);

    // Remove from table
    table.state->cpuHeapTable.Remove(type, cpuDescriptorBase.ptr);

    // GPU visibility optional
    if (flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
        table.state->cpuHeapTable.Remove(type, gpuDescriptorBase.ptr);
    }

    // Release table
    destroy(prmTable, allocators);

    // Release parent
    parent->Release();
}

uint32_t DescriptorHeapState::GetOffsetFromHeapHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) const {
    // Get base offset from heap
    uint64_t offset = handle.ptr - cpuDescriptorBase.ptr;
    ASSERT(offset % stride == 0, "Invalid heap offset");

    // As offset
    return static_cast<uint32_t>(offset / stride);
}

uint32_t DescriptorHeapState::GetOffsetFromHeapHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) const {
    // Get base offset from heap
    uint64_t offset = handle.ptr - gpuDescriptorBase.ptr;
    ASSERT(offset % stride == 0, "Invalid heap offset");

    // As offset
    return static_cast<uint32_t>(offset / stride);
}

ResourceState * DescriptorHeapState::GetStateFromHeapHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) const {
    // Get from PRMT
    return prmTable->GetMappingState(GetOffsetFromHeapHandle(handle));
}

ResourceState * DescriptorHeapState::GetStateFromHeapHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) const {
    return prmTable->GetMappingState(GetOffsetFromHeapHandle(handle));
}

VirtualResourceMapping DescriptorHeapState::GetVirtualMappingFromHeapHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) const {
    return prmTable->GetMapping(GetOffsetFromHeapHandle(handle));
}

VirtualResourceMapping DescriptorHeapState::GetVirtualMappingFromHeapHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) const {
    return prmTable->GetMapping(GetOffsetFromHeapHandle(handle));
}

VirtualResourceMapping DescriptorHeapState::GetVirtualMappingFromHeapHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle, ResourceState** state) const {
    return prmTable->GetMapping(GetOffsetFromHeapHandle(handle), state);
}

VirtualResourceMapping DescriptorHeapState::GetVirtualMappingFromHeapHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle, ResourceState** state) const {
    return prmTable->GetMapping(GetOffsetFromHeapHandle(handle), state);
}
