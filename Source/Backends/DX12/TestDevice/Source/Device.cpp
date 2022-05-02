#include <Test/Device/DX12/Device.h>
#include <Test/Device/Catch2.h>

using namespace Test;
using namespace Test::DX12;

Device::~Device() {

}

void Device::Install(const DeviceInfo &info) {

}

QueueID Device::GetQueue(QueueType type) {
    return Test::QueueID();
}

BufferID Device::CreateTexelBuffer(ResourceType type, Backend::IL::Format format, uint64_t size, void *data) {
    return Test::BufferID();
}

TextureID Device::CreateTexture(ResourceType type, Backend::IL::Format format, uint32_t width, uint32_t height, uint32_t depth, void *data) {
    return Test::TextureID();
}

ResourceLayoutID Device::CreateResourceLayout(const ResourceType *types, uint32_t count) {
    return Test::ResourceLayoutID();
}

ResourceSetID Device::CreateResourceSet(ResourceLayoutID layout, const ResourceID *resources, uint32_t count) {
    return Test::ResourceSetID();
}

PipelineID Device::CreateComputePipeline(const ResourceLayoutID *layouts, uint32_t layoutCount, const void *shaderCode, uint32_t shaderSize) {
    return Test::PipelineID();
}

CommandBufferID Device::CreateCommandBuffer(QueueType type) {
    return Test::CommandBufferID();
}

void Device::BeginCommandBuffer(CommandBufferID commandBuffer) {

}

void Device::EndCommandBuffer(CommandBufferID commandBuffer) {

}

void Device::BindPipeline(CommandBufferID commandBuffer, PipelineID pipeline) {

}

void Device::BindResourceSet(CommandBufferID commandBuffer, ResourceSetID resourceSet) {

}

void Device::Dispatch(CommandBufferID commandBuffer, uint32_t x, uint32_t y, uint32_t z) {

}

void Device::Submit(QueueID queue, CommandBufferID commandBuffer) {

}

void Device::InitializeResources(CommandBufferID commandBuffer) {

}

void Device::Flush() {

}
