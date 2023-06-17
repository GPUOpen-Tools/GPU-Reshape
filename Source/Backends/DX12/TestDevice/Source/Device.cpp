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

#include <Test/Device/DX12/Device.h>
#include <Test/Device/Catch2.h>

// Layer
#include <Backends/DX12/Layer.h>
#include <Backends/DX12/Translation.h>

using namespace Test;
using namespace Test::DX12;

Device::~Device() {

}

void Device::Install(const DeviceInfo &info) {
    REQUIRE(LoadLibraryA("GRS.Backends.DX12.Layer.dll") != nullptr);

    // Create the underlying device
    CreateDevice();

    // Create the standard set of queues
    CreateSharedQueues();

    // Created shared heaps
    CreateSharedHeaps();

    // Create shared fence
    CreateSharedFence();
}

void Device::CreateDevice() {
    UINT dxgiFactoryFlags = 0;

    // Validation
#ifndef NDEBUG
    {
        ComPtr <ID3D12Debug> debugController;

        // Debug controller present?
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif // NDEBUG

    // Create factory
    ComPtr <IDXGIFactory4> factory;
    REQUIRE(SUCCEEDED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory))));

    // Revision 6?
    ComPtr <IDXGIFactory6> factory6;
    if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory6)))) {
        for (UINT i = 0; SUCCEEDED(factory6->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter))); ++i) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
                break;
            }
        }
    }

    // None available?
    if (adapter.Get() == nullptr) {
        for (UINT i = 0; SUCCEEDED(factory->EnumAdapters1(i, &adapter)); ++i) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
                break;
            }
        }
    }

    // Pass down the environment
    D3D12_DEVICE_GPUOPEN_GPU_VALIDATION_INFO gpuOpenInfo{};
    gpuOpenInfo.registry = registry;

    // Create device
    REQUIRE(SUCCEEDED(D3D12CreateDeviceGPUOpen(
        adapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&device),
        &gpuOpenInfo
    )));
}

void Device::CreateSharedQueues() {
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    // Graphics
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    REQUIRE(SUCCEEDED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&graphicsQueue))));

    // Compute
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    REQUIRE(SUCCEEDED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&computeQueue))));

    // Copy
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    REQUIRE(SUCCEEDED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&copyQueue))));

    // Create shared allocator
    REQUIRE(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator))));
}

void Device::CreateSharedHeaps() {
    CreateSharedHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1'000'000, sharedResourceHeap);
    CreateSharedHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 32, sharedSamplerHeap);
}

void Device::CreateSharedHeap(D3D12_DESCRIPTOR_HEAP_TYPE heap, uint32_t count, Device::HeapInfo &info) {
    // Create heap
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = count;
    heapDesc.Type = heap;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    REQUIRE(SUCCEEDED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&info.sharedHeap))));

    // Get offsets
    info.sharedCPUHeapOffset = info.sharedHeap->GetCPUDescriptorHandleForHeapStart();
    info.sharedGPUHeapOffset = info.sharedHeap->GetGPUDescriptorHandleForHeapStart();

    // Get stride
    info.sharedHeapStride = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void Device::CreateSharedFence() {
    // Create shared fence
    REQUIRE(SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&waitFence))));

    // Create shared event
    waitFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    REQUIRE(waitFenceEvent != nullptr);
}

QueueID Device::GetQueue(QueueType type) {
    switch (type) {
        case QueueType::Graphics:
            return QueueID(0);
        case QueueType::Compute:
            return QueueID(1);
        case QueueType::Transfer:
            return QueueID(2);
    }

    return QueueID::Invalid();
}

BufferID Device::CreateTexelBuffer(ResourceType type, Backend::IL::Format format, uint64_t size, const void *data, uint64_t dataSize) {
    ResourceInfo &resource = resources.emplace_back();
    resource.type = type;
    resource.format = format;

    // Destination heap
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    // Description
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.Width = size;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    // Create resource
    REQUIRE(SUCCEEDED(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&resource.resource))));

    // Any data to upload?
    if (data && dataSize) {
        UploadBuffer& uploadBuffer = CreateUploadBuffer(dataSize);

        UINT8* mapped;

        // Map range
        D3D12_RANGE readRange{ 0, 0 };
        REQUIRE(SUCCEEDED(uploadBuffer.resource->Map(0, &readRange, reinterpret_cast<void**>(&mapped))));

        // Write data
        memcpy(mapped, data, sizeof(dataSize));
        uploadBuffer.resource->Unmap(0, nullptr);

        // Enqueue command
        UpdateCommand command;
        command.copyBuffer.type = UpdateCommandType::CopyBuffer;
        command.copyBuffer.source = uploadBuffer.resource.Get();
        command.copyBuffer.dest = resource.resource.Get();
        command.copyBuffer.dataSize = dataSize;
        updateCommands.push_back(command);
    }

    return BufferID(ResourceID(static_cast<uint32_t>(resources.size()) - 1));
}

TextureID Device::CreateTexture(ResourceType type, Backend::IL::Format format, uint32_t width, uint32_t height, uint32_t depth, const void *data, uint64_t dataSize) {
    ResourceInfo &resource = resources.emplace_back();
    resource.type = type;
    resource.format = format;

    // Destination heap
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    // Description
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Format = Translate(format);
    resourceDesc.Width = width;
    resourceDesc.Height = height;
    resourceDesc.DepthOrArraySize = depth;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    // Translate type
    switch (type) {
        default:
        ASSERT(false, "Invalid resource type");
            break;
        case ResourceType::Texture1D:
        case ResourceType::RWTexture1D:
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            break;
        case ResourceType::Texture2D:
        case ResourceType::RWTexture2D:
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;
        case ResourceType::Texture3D:
        case ResourceType::RWTexture3D:
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            break;
    }

    // Create resource
    REQUIRE(SUCCEEDED(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&resource.resource))));

    return TextureID(ResourceID(static_cast<uint32_t>(resources.size()) - 1));
}

ResourceLayoutID Device::CreateResourceLayout(const ResourceType *types, uint32_t count, bool isLastUnbounded) {
    ResourceLayoutInfo &layout = resourceLayouts.emplace_back();

    // Translate bindings
    for (uint32_t i = 0; i < count; i++) {
        // Special case for sampler
        if (types[i] == ResourceType::StaticSamplerState) {
            auto& sampler = layout.staticSamplers.emplace_back(D3D12_STATIC_SAMPLER_DESC{});
            sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
            sampler.RegisterSpace = 0;
            sampler.ShaderRegister = i;
            continue;
        }

        auto &range = layout.ranges.emplace_back(D3D12_DESCRIPTOR_RANGE{});
        range.RegisterSpace = 0;
        range.BaseShaderRegister = i;
        range.NumDescriptors = 1;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // Translate type
        switch (types[i]) {
            default:
                ASSERT(false, "Invalid type");
            case ResourceType::TexelBuffer:
            case ResourceType::Texture1D:
            case ResourceType::Texture2D:
            case ResourceType::Texture3D:
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                break;
            case ResourceType::RWTexelBuffer:
            case ResourceType::RWTexture1D:
            case ResourceType::RWTexture2D:
            case ResourceType::RWTexture3D:
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                break;
            case ResourceType::CBuffer:
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                break;
            case ResourceType::SamplerState:
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                break;
        }
    }

    return ResourceLayoutID(static_cast<uint32_t>(resourceLayouts.size()) - 1);
}

ResourceSetID Device::CreateResourceSet(ResourceLayoutID layout, const ResourceID *setResources, uint32_t count) {
    ResourceSetInfo &set = resourceSets.emplace_back();

    // Validate layout size
    const ResourceLayoutInfo &_layout = resourceLayouts[layout];
    REQUIRE(count == _layout.ranges.size() + _layout.staticSamplers.size());

    // Set offsets
    if (_layout.ranges[0].RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
        set.heapCPUHandleOffset = sharedSamplerHeap.sharedCPUHeapOffset;
        set.heapGPUHandleOffset = sharedSamplerHeap.sharedGPUHeapOffset;
    } else {
        set.heapCPUHandleOffset = sharedResourceHeap.sharedCPUHeapOffset;
        set.heapGPUHandleOffset = sharedResourceHeap.sharedGPUHeapOffset;
    }

    // Create all resources
    for (uint32_t i = 0; i < count; i++) {
        const ResourceInfo &resource = resources[setResources[i]];

        // Ignore static samplers
        if (resource.type == ResourceType::SamplerState && _layout.ranges[0].RangeType != D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
            continue;
        }

        // Get description
        D3D12_RESOURCE_DESC resourceDesc;
        if (resource.type != ResourceType::SamplerState) {
            resourceDesc = resource.resource->GetDesc();
        }

        // Create view
        switch (resource.type) {
            default: {
                ASSERT(false, "Unexpected resource type");
                break;
            }
            case ResourceType::TexelBuffer: {
                D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
                desc.Format = Translate(resource.format);
                desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                desc.Buffer.FirstElement = 0;
                desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
                desc.Buffer.NumElements = static_cast<uint32_t>(resourceDesc.Width / Backend::IL::GetSize(resource.format));
                desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                device->CreateShaderResourceView(resource.resource.Get(), &desc, sharedResourceHeap.sharedCPUHeapOffset);
                set.count++;
                break;
            }
            case ResourceType::RWTexelBuffer: {
                D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
                desc.Format = Translate(resource.format);
                desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                desc.Buffer.FirstElement = 0;
                desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
                desc.Buffer.NumElements = static_cast<uint32_t>(resourceDesc.Width / Backend::IL::GetSize(resource.format));
                device->CreateUnorderedAccessView(resource.resource.Get(), nullptr, &desc, sharedResourceHeap.sharedCPUHeapOffset);
                set.count++;
                break;
            }
            case ResourceType::Texture1D: {
                D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
                desc.Format = resourceDesc.Format;
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                desc.Texture1D.MipLevels = 1;
                device->CreateShaderResourceView(resource.resource.Get(), &desc, sharedResourceHeap.sharedCPUHeapOffset);
                set.count++;
                break;
            }
            case ResourceType::RWTexture1D: {
                D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
                desc.Format = resourceDesc.Format;
                desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                device->CreateUnorderedAccessView(resource.resource.Get(), nullptr, &desc, sharedResourceHeap.sharedCPUHeapOffset);
                set.count++;
                break;
            }
            case ResourceType::Texture2D: {
                D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
                desc.Format = resourceDesc.Format;
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                desc.Texture2D.MipLevels = 1;
                device->CreateShaderResourceView(resource.resource.Get(), &desc, sharedResourceHeap.sharedCPUHeapOffset);
                set.count++;
                break;
            }
            case ResourceType::RWTexture2D: {
                D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
                desc.Format = resourceDesc.Format;
                desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                device->CreateUnorderedAccessView(resource.resource.Get(), nullptr, &desc, sharedResourceHeap.sharedCPUHeapOffset);
                set.count++;
                break;
            }
            case ResourceType::Texture3D: {
                D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
                desc.Format = resourceDesc.Format;
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                desc.Texture3D.MipLevels = 1;
                device->CreateShaderResourceView(resource.resource.Get(), &desc, sharedResourceHeap.sharedCPUHeapOffset);
                set.count++;
                break;
            }
            case ResourceType::RWTexture3D: {
                D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
                desc.Format = resourceDesc.Format;
                desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                device->CreateUnorderedAccessView(resource.resource.Get(), nullptr, &desc, sharedResourceHeap.sharedCPUHeapOffset);
                set.count++;
                break;
            }
            case ResourceType::CBuffer: {
                D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
                desc.BufferLocation = resource.resource->GetGPUVirtualAddress();
                desc.SizeInBytes = static_cast<uint32_t>(resourceDesc.Width);
                device->CreateConstantBufferView(&desc, sharedResourceHeap.sharedCPUHeapOffset);
                set.count++;
                break;
            }
            case ResourceType::SamplerState: {
                D3D12_SAMPLER_DESC sampler{};
                sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
                sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
                sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
                sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
                device->CreateSampler(&sampler, sharedSamplerHeap.sharedCPUHeapOffset);
                continue;
            }
        }

        // Offset heap
        HeapInfo& heap = GetHeapForType(resource.type);
        heap.sharedCPUHeapOffset.ptr += heap.sharedHeapStride;
        heap.sharedGPUHeapOffset.ptr += heap.sharedHeapStride;
    }

    // OK
    return ResourceSetID(static_cast<uint32_t>(resourceSets.size()) - 1);
}

PipelineID Device::CreateComputePipeline(const ResourceLayoutID *layouts, uint32_t layoutCount, const void *shaderCode, uint64_t shaderSize) {
    PipelineInfo &pipeline = pipelines.emplace_back();

    // All root parameters
    std::vector<D3D12_ROOT_PARAMETER> rootParameters;
    rootParameters.resize(layoutCount);

    // All root samplers
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
    rootParameters.resize(layoutCount);

    // Deduce range count
    uint32_t rangeCount{0};
    for (uint32_t i = 0; i < layoutCount; i++) {
        const ResourceLayoutInfo &layout = resourceLayouts[i];
        rangeCount += static_cast<uint32_t>(layout.ranges.size());
    }

    // Range data
    std::vector<D3D12_DESCRIPTOR_RANGE> ranges(rangeCount);

    // Translate parameters
    for (uint32_t i = 0, rangeOffset = 0; i < layoutCount; i++) {
        const ResourceLayoutInfo &layout = resourceLayouts[i];

        // Copy ranges and select space
        for (uint64_t j = 0; j < layout.ranges.size(); j++) {
            D3D12_DESCRIPTOR_RANGE& range = ranges[rangeOffset + j] = layout.ranges[j];
            range.RegisterSpace = i;
        }

        D3D12_ROOT_PARAMETER &root = rootParameters[i];
        root.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        root.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        root.DescriptorTable.NumDescriptorRanges = static_cast<uint32_t>(layout.ranges.size());
        root.DescriptorTable.pDescriptorRanges = &ranges[rangeOffset];

        // Inherit samplers
        staticSamplers.insert(staticSamplers.end(), layout.staticSamplers.begin(), layout.staticSamplers.end());

        // Next offset
        rangeOffset += static_cast<uint32_t>(layout.ranges.size());
    }

    // Signature description
    D3D12_ROOT_SIGNATURE_DESC signatureDesc{};
    signatureDesc.NumParameters = static_cast<uint32_t>(rootParameters.size());
    signatureDesc.pParameters = rootParameters.data();
    signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    signatureDesc.NumStaticSamplers = static_cast<uint32_t>(staticSamplers.size());
    signatureDesc.pStaticSamplers = staticSamplers.data();

    // Serialize signature
    ComPtr <ID3DBlob> pOutBlob;
    ComPtr <ID3DBlob> pErrorBlob;
    HRESULT status = D3D12SerializeRootSignature(
        &signatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        pOutBlob.GetAddressOf(),
        pErrorBlob.GetAddressOf()
    );

    // Did we fail?
    if (FAILED(status)) {
        REQUIRE_FORMAT(false, static_cast<const char*>(pErrorBlob->GetBufferPointer()));
    }

    // Create signature
    REQUIRE(SUCCEEDED(device->CreateRootSignature(
        0x1,
        pOutBlob->GetBufferPointer(),
        pOutBlob->GetBufferSize(),
        IID_PPV_ARGS(&pipeline.rootSignature)
    )));

    // Create pipeline
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = pipeline.rootSignature.Get();
    desc.CS.pShaderBytecode = shaderCode;
    desc.CS.BytecodeLength = shaderSize;
    REQUIRE(SUCCEEDED(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline.pipeline))));

    return PipelineID(static_cast<uint32_t>(pipelines.size()) - 1);
}

CommandBufferID Device::CreateCommandBuffer(QueueType type) {
    CommandBufferInfo& info = commandBuffers.emplace_back();

    // Create list
    REQUIRE(SUCCEEDED(device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&info.commandList)
    )));

    // Close list, open by default
    info.commandList->Close();

    return Test::CommandBufferID(static_cast<uint32_t>(commandBuffers.size()) - 1);
}

void Device::BeginCommandBuffer(CommandBufferID commandBuffer) {
    CommandBufferInfo& info = commandBuffers.at(commandBuffer);
    info.context = {};

    // Attempt to reset
    REQUIRE(SUCCEEDED(info.commandList->Reset(commandAllocator.Get(), nullptr)));

    // Set heaps
    ID3D12DescriptorHeap* heaps[] = { sharedResourceHeap.sharedHeap.Get(), sharedSamplerHeap.sharedHeap.Get() };
    info.commandList->SetDescriptorHeaps(2, heaps);
}

void Device::EndCommandBuffer(CommandBufferID commandBuffer) {
    CommandBufferInfo& info = commandBuffers.at(commandBuffer);
    info.commandList->Close();
}

void Device::BindPipeline(CommandBufferID commandBuffer, PipelineID pipeline) {
    CommandBufferInfo& info = commandBuffers.at(commandBuffer);

    // Get pipeline
    const PipelineInfo& pipelineInfo = pipelines[pipeline];

    // Set states
    info.commandList->SetComputeRootSignature(pipelineInfo.rootSignature.Get());
    info.commandList->SetPipelineState(pipelineInfo.pipeline.Get());
}

void Device::BindResourceSet(CommandBufferID commandBuffer, uint32_t slot, ResourceSetID resourceSet) {
    CommandBufferInfo& info = commandBuffers.at(commandBuffer);
    info.commandList->SetComputeRootDescriptorTable(slot, resourceSets[resourceSet].heapGPUHandleOffset);
}

void Device::Dispatch(CommandBufferID commandBuffer, uint32_t x, uint32_t y, uint32_t z) {
    CommandBufferInfo& info = commandBuffers.at(commandBuffer);
    info.commandList->Dispatch(x, y, z);
}

void Device::Submit(QueueID queueID, CommandBufferID commandBuffer) {
    CommandBufferInfo& info = commandBuffers.at(commandBuffer);

    // Determine the queue
    ID3D12CommandQueue* queue{};
    switch (queueID) {
        case 0:
            queue = graphicsQueue.Get();
            break;
        case 1:
            queue = computeQueue.Get();
            break;
        case 2:
            queue = copyQueue.Get();
            break;
    }

    // Must be valid
    REQUIRE(queue);

    // Submit on respective queue
    ID3D12CommandList *lists[] = {info.commandList.Get()};
    queue->ExecuteCommandLists(1, lists);
}

void Device::InitializeResources(CommandBufferID commandBuffer) {
    CommandBufferInfo& info = commandBuffers.at(commandBuffer);

    // Handle commands
    for (const UpdateCommand& cmd : updateCommands) {
        switch (cmd.type) {
            case UpdateCommandType::CopyBuffer: {
                info.commandList->CopyBufferRegion(
                    cmd.copyBuffer.dest,
                    0u,
                    cmd.copyBuffer.source,
                    0u,
                    cmd.copyBuffer.dataSize
                );
                break;
            }
        }
    }
}

void Device::Flush() {
    graphicsQueue->Signal(waitFence.Get(), ++waitFenceCounter);

    // Wait graphics
    if (waitFence->GetCompletedValue() != waitFenceCounter) {
        REQUIRE(SUCCEEDED(waitFence->SetEventOnCompletion(waitFenceCounter, waitFenceEvent)));
        WaitForSingleObject(waitFenceEvent, INFINITE);
    }

    computeQueue->Signal(waitFence.Get(), ++waitFenceCounter);

    // Wait compute
    if (waitFence->GetCompletedValue() != waitFenceCounter) {
        REQUIRE(SUCCEEDED(waitFence->SetEventOnCompletion(waitFenceCounter, waitFenceEvent)));
        WaitForSingleObject(waitFenceEvent, INFINITE);
    }

    copyQueue->Signal(waitFence.Get(), ++waitFenceCounter);

    // Wait copy
    if (waitFence->GetCompletedValue() != waitFenceCounter) {
        REQUIRE(SUCCEEDED(waitFence->SetEventOnCompletion(waitFenceCounter, waitFenceEvent)));
        WaitForSingleObject(waitFenceEvent, INFINITE);
    }

    // Let the backend catch up to the messages
    graphicsQueue->ExecuteCommandLists(0u, nullptr);
    computeQueue->ExecuteCommandLists(0u, nullptr);
}

SamplerID Device::CreateSampler() {
    ResourceInfo& resource = resources.emplace_back();
    resource.type = ResourceType::SamplerState;
    return SamplerID(ResourceID(static_cast<uint32_t>(resources.size()) - 1));
}

Device::HeapInfo &Device::GetHeapForType(ResourceType type) {
    switch (type) {
        default:
            ASSERT(false, "Invalid resource type");
            return sharedResourceHeap;
        case ResourceType::TexelBuffer:
        case ResourceType::RWTexelBuffer:
        case ResourceType::Texture1D:
        case ResourceType::RWTexture1D:
        case ResourceType::Texture2D:
        case ResourceType::RWTexture2D:
        case ResourceType::Texture3D:
        case ResourceType::RWTexture3D:
        case ResourceType::CBuffer:
            return sharedResourceHeap;
        case ResourceType::SamplerState:
            return sharedSamplerHeap;
    }
}

CBufferID Device::CreateCBuffer(uint32_t byteSize, const void *data, uint64_t dataSize) {
    ResourceInfo &resource = resources.emplace_back();
    resource.type = ResourceType::CBuffer;

    // Destination heap
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    // Description
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.Width = std::max(256u, byteSize);
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    // Create resource
    REQUIRE(SUCCEEDED(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&resource.resource))));

    // Any data to upload?
    if (data) {
        UploadBuffer& uploadBuffer = CreateUploadBuffer(dataSize);

        UINT8* mapped;

        // Map range
        D3D12_RANGE readRange{ 0, 0 };
        REQUIRE(SUCCEEDED(uploadBuffer.resource->Map(0, &readRange, reinterpret_cast<void**>(&mapped))));

        // Write data
        memcpy(mapped, data, sizeof(dataSize));
        uploadBuffer.resource->Unmap(0, nullptr);

        // Enqueue command
        UpdateCommand command;
        command.copyBuffer.type = UpdateCommandType::CopyBuffer;
        command.copyBuffer.source = uploadBuffer.resource.Get();
        command.copyBuffer.dest = resource.resource.Get();

        command.copyBuffer.dataSize = dataSize;
        updateCommands.push_back(command);
    }

    return CBufferID(ResourceID(static_cast<uint32_t>(resources.size()) - 1));
}

Device::UploadBuffer &Device::CreateUploadBuffer(uint64_t size) {
    UploadBuffer& uploadBuffer = uploadBuffers.emplace_back();

    // Destination heap
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    // Description
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.Width = std::max(256ull, size);
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    // Create resource
    REQUIRE(SUCCEEDED(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer.resource))));

    return uploadBuffer;
}

const char *Device::GetName() {
    return "D3D12";
}
