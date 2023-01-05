#include <Backends/DX12/Export/ShaderExportStreamAllocator.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>

// Backend
#include <Backend/IShaderExportHost.h>

// Common
#include <Common/Assert.h>
#include <Common/Registry.h>

// Bridge
#include <Common/Format.h>

// Std
#include <algorithm>

ShaderExportStreamAllocator::ShaderExportStreamAllocator() {

}

bool ShaderExportStreamAllocator::Install() {
    deviceAllocator = registry->Get<DeviceAllocator>();

    auto host = registry->Get<IShaderExportHost>();

    // Get the number of exports
    uint32_t exportCount;
    host->Enumerate(&exportCount, nullptr);

    // Enumerate all exports
    std::vector<ShaderExportID> exportIDs(exportCount);
    host->Enumerate(&exportCount, exportIDs.data());

    // Allocate features
    exportInfos.resize(host->GetBound());

    // Initialize all feature infos
    for (ShaderExportID id : exportIDs) {
        ExportInfo& info = exportInfos[id];
        info.id = id;
        info.typeInfo = host->GetTypeInfo(id);
        info.dataSize = baseDataSize;
    }

    return true;
}

ShaderExportStreamAllocator::~ShaderExportStreamAllocator() {
    for (ShaderExportSegmentInfo* segment : segmentPool) {
        // Release streams
        for (const ShaderExportStreamInfo& stream : segment->streams) {
            deviceAllocator->Free(stream.allocation);
        }

        // Release counter
        deviceAllocator->Free(segment->counter.allocation);
    }
}

ShaderExportSegmentInfo *ShaderExportStreamAllocator::AllocateSegment() {
    // Try existing allocation
    if (ShaderExportSegmentInfo* segment = segmentPool.TryPop()) {
        return segment;
    }

    // Allocate new allocation
    auto segment = new (allocators) ShaderExportSegmentInfo();

    // Allocate counters
    segment->counter = AllocateCounterInfo();

    // Set number of streams
    segment->streams.resize(exportInfos.size());

    // Allocate all streams
    for (const ExportInfo& exportInfo : exportInfos) {
        segment->streams[exportInfo.id] = AllocateStreamInfo(exportInfo.id);
    }

#if LOG_ALLOCATION
    table->parent->logBuffer.Add("Vulkan", Format("Allocated segment with {} streams", segment->streams.size()));
#endif

    // OK
    return segment;
}

void ShaderExportStreamAllocator::FreeSegment(ShaderExportSegmentInfo *segment) {
    segmentPool.Push(segment);
}

void ShaderExportStreamAllocator::SetStreamSize(ShaderExportID id, uint64_t size) {

}

ShaderExportSegmentCounterInfo ShaderExportStreamAllocator::AllocateCounterInfo() {
    ShaderExportSegmentCounterInfo info{};

    // Attempt to re-use an existing allocation
    if (counterPool.TryPop(info)) {
        return info;
    }

    // Buffer info
    D3D12_RESOURCE_DESC bufferDesc{};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = sizeof(ShaderExportCounter) * std::max(1ull, exportInfos.size());
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.SampleDesc.Count = 1;

    // Create allocation
    info.allocation = deviceAllocator->AllocateMirror(bufferDesc);

    // Setup view
    info.view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    info.view.Format = DXGI_FORMAT_R32_UINT;
    info.view.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    info.view.Buffer.NumElements = std::max(1ull, exportInfos.size());
    info.view.Buffer.FirstElement = 0;

    // OK
    return info;
}

ShaderExportStreamInfo ShaderExportStreamAllocator::AllocateStreamInfo(const ShaderExportID& id) {
    // Get the export info
    ExportInfo& exportInfo = exportInfos[id];

    // Attempt to re-use an existing allocation
    ShaderExportStreamInfo info{};
    if (streamPool.TryPop(info)) {
        return info;
    }

    // Inherit type info
    info.typeInfo = exportInfo.typeInfo;

    // Buffer info
    D3D12_RESOURCE_DESC bufferDesc{};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = exportInfo.dataSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.SampleDesc.Count = 1;

    // Create allocation
    info.allocation = deviceAllocator->AllocateMirror(bufferDesc, AllocationResidency::Host);

    // Size for safe guarding
    info.byteSize = exportInfo.dataSize;

    // Setup view
    info.view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    info.view.Format = DXGI_FORMAT_R32_UINT;
    info.view.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    info.view.Buffer.NumElements = exportInfo.dataSize / sizeof(uint32_t);
    info.view.Buffer.FirstElement = 0;

    // OK
    return info;
}
