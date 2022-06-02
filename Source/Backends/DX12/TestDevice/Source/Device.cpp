#include <Test/Device/DX12/Device.h>
#include <Test/Device/Catch2.h>

// Layer
#include <Backends/DX12/Translation.h>

using namespace Test;
using namespace Test::DX12;

Device::~Device() {

}

void Device::Install(const DeviceInfo &info) {
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

    // Create device
    REQUIRE(SUCCEEDED(D3D12CreateDevice(
        adapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&device)
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
    // Create heap
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 2048;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    REQUIRE(SUCCEEDED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&sharedHeap))));

    // Get offsets
    sharedCPUHeapOffset = sharedHeap->GetCPUDescriptorHandleForHeapStart();
    sharedGPUHeapOffset = sharedHeap->GetGPUDescriptorHandleForHeapStart();

    // Get stride
    sharedHeapStride = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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

BufferID Device::CreateTexelBuffer(ResourceType type, Backend::IL::Format format, uint64_t size, void *data) {
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

    // TODO: Staging!
    if (data != nullptr) {
        UINT8* mapped;

        // Map range
        D3D12_RANGE readRange{ 0, 0 };
        REQUIRE(SUCCEEDED(resource.resource->Map(0, &readRange, reinterpret_cast<void**>(&mapped))));

        // Write data
        memcpy(mapped, data, sizeof(size));
        resource.resource->Unmap(0, nullptr);
    }

    return BufferID(ResourceID(static_cast<uint32_t>(resources.size()) - 1));
}

TextureID Device::CreateTexture(ResourceType type, Backend::IL::Format format, uint32_t width, uint32_t height, uint32_t depth, void *data) {
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

ResourceLayoutID Device::CreateResourceLayout(const ResourceType *types, uint32_t count) {
    ResourceLayoutInfo &layout = resourceLayouts.emplace_back();
    layout.ranges.resize(count);

    // Translate bindings
    for (uint32_t i = 0; i < count; i++) {
        D3D12_DESCRIPTOR_RANGE &range = (layout.ranges[i] = {});
        range.RegisterSpace = 0;
        range.BaseShaderRegister = i;
        range.NumDescriptors = 1;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        // Translate type
        switch (types[i]) {
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
        }
    }

    return ResourceLayoutID(static_cast<uint32_t>(resourceLayouts.size()) - 1);
}

ResourceSetID Device::CreateResourceSet(ResourceLayoutID layout, const ResourceID *setResources, uint32_t count) {
    ResourceSetInfo &set = resourceSets.emplace_back();

    // Validate layout size
    const ResourceLayoutInfo &_layout = resourceLayouts[layout];
    REQUIRE(count == _layout.ranges.size());

    // Set offset
    set.heapCPUHandleOffset = sharedCPUHeapOffset;
    set.heapGPUHandleOffset = sharedGPUHeapOffset;

    // Create all resources
    for (uint32_t i = 0; i < count; i++) {
        const ResourceInfo &resource = resources[setResources[i]];

        // Get description
        D3D12_RESOURCE_DESC resourceDesc = resource.resource->GetDesc();

        // Create view
        switch (resource.type) {
            case ResourceType::TexelBuffer: {
                D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
                desc.Format = Translate(resource.format);
                desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                desc.Buffer.FirstElement = 0;
                desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
                desc.Buffer.NumElements = resourceDesc.Width / Backend::IL::GetSize(resource.format);
                desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                device->CreateShaderResourceView(resource.resource.Get(), &desc, sharedCPUHeapOffset);
                break;
            }
            case ResourceType::RWTexelBuffer: {
                D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
                desc.Format = Translate(resource.format);
                desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                desc.Buffer.FirstElement = 0;
                desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
                desc.Buffer.NumElements = resourceDesc.Width / Backend::IL::GetSize(resource.format);
                device->CreateUnorderedAccessView(resource.resource.Get(), nullptr, &desc, sharedCPUHeapOffset);
                break;
            }
            case ResourceType::Texture1D: {
                D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
                desc.Format = resourceDesc.Format;
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                desc.Texture1D.MipLevels = 1;
                device->CreateShaderResourceView(resource.resource.Get(), &desc, sharedCPUHeapOffset);
                break;
            }
            case ResourceType::RWTexture1D: {
                D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
                desc.Format = resourceDesc.Format;
                desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                device->CreateUnorderedAccessView(resource.resource.Get(), nullptr, &desc, sharedCPUHeapOffset);
                break;
            }
            case ResourceType::Texture2D: {
                D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
                desc.Format = resourceDesc.Format;
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                desc.Texture2D.MipLevels = 1;
                device->CreateShaderResourceView(resource.resource.Get(), &desc, sharedCPUHeapOffset);
                break;
            }
            case ResourceType::RWTexture2D: {
                D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
                desc.Format = resourceDesc.Format;
                desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                device->CreateUnorderedAccessView(resource.resource.Get(), nullptr, &desc, sharedCPUHeapOffset);
                break;
            }
            case ResourceType::Texture3D: {
                D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
                desc.Format = resourceDesc.Format;
                desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                desc.Texture3D.MipLevels = 1;
                device->CreateShaderResourceView(resource.resource.Get(), &desc, sharedCPUHeapOffset);
                break;
            }
            case ResourceType::RWTexture3D: {
                D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
                desc.Format = resourceDesc.Format;
                desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                device->CreateUnorderedAccessView(resource.resource.Get(), nullptr, &desc, sharedCPUHeapOffset);
                break;
            }
        }

        // Offset heap
        sharedCPUHeapOffset.ptr += sharedHeapStride;
        sharedGPUHeapOffset.ptr += sharedHeapStride;
    }

    // OK
    return ResourceSetID(static_cast<uint32_t>(resourceSets.size()) - 1);
}

PipelineID Device::CreateComputePipeline(const ResourceLayoutID *layouts, uint32_t layoutCount, const void *shaderCode, uint32_t shaderSize) {
    PipelineInfo &pipeline = pipelines.emplace_back();

    // All root parameters
    std::vector<D3D12_ROOT_PARAMETER> rootParameters;
    rootParameters.resize(layoutCount);

    // Translate parameters
    for (uint32_t i = 0; i < layoutCount; i++) {
        const ResourceLayoutInfo &layout = resourceLayouts[i];

        D3D12_ROOT_PARAMETER &root = rootParameters[i];
        root.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        root.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        root.DescriptorTable.NumDescriptorRanges = layout.ranges.size();
        root.DescriptorTable.pDescriptorRanges = layout.ranges.data();
    }

    // Signature description
    D3D12_ROOT_SIGNATURE_DESC signatureDesc{};
    signatureDesc.NumParameters = rootParameters.size();
    signatureDesc.pParameters = rootParameters.data();
    signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    // Serialize signature
    ComPtr <ID3DBlob> pOutBlob;
    ComPtr <ID3DBlob> pErrorBlob;
    REQUIRE(SUCCEEDED(D3D12SerializeRootSignature(
        &signatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        pOutBlob.GetAddressOf(),
        pErrorBlob.GetAddressOf()
    )));

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
    ID3D12DescriptorHeap* heaps[] = { sharedHeap.Get() };
    info.commandList->SetDescriptorHeaps(1, heaps);
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

void Device::BindResourceSet(CommandBufferID commandBuffer, ResourceSetID resourceSet) {
    CommandBufferInfo& info = commandBuffers.at(commandBuffer);
    info.commandList->SetComputeRootDescriptorTable(0, resourceSets[resourceSet].heapGPUHandleOffset);
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
    /* poof */
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
}
