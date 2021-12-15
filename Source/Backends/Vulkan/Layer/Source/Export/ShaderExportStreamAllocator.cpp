#include <Backends/Vulkan/Export/ShaderExportStreamAllocator.h>

#include <Common/Assert.h>

ShaderExportStreamAllocator::ShaderExportStreamAllocator(DeviceDispatchTable *table) : table(table) {

}

void ShaderExportStreamAllocator::SetAllocationMode(ShaderExportAllocationMode mode) {
    allocationMode = mode;
}

ShaderExportStream ShaderExportStreamAllocator::Allocate(ShaderExportID id, const ShaderExportTypeInfo &typeInfo) {
    StreamInfo& info = GetStreamInfo(id);

    // Create if not ready
    if (!info.size) {
        CreateStream(info);
    }

    if (info.cou)

    return info.stream;
}

void ShaderExportStreamAllocator::SetCounter(ShaderExportID id, uint64_t counter) {
    StreamInfo& info = GetStreamInfo(id);
    ASSERT(info.size, "Invalid stream");

    info.counter = counter;
}

ShaderExportStreamAllocator::StreamInfo &ShaderExportStreamAllocator::GetStreamInfo(ShaderExportID id) {
    if (streams.size() <= id) {
        streams.resize(id + 1);
    }

    return streams[id];
}

void ShaderExportStreamAllocator::CreateStream(ShaderExportStreamAllocator::StreamInfo &info) {

}
