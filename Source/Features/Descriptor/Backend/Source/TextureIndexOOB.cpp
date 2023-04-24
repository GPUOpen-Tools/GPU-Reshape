#include <Test/Device/IDevice.h>
#include <Test/Device/ShaderHost.h>

void TextureIndexOOBExecutor(Test::IDevice *device) {
    using namespace Test;

    ResourceLayoutID layout = device->CreateResourceLayout({
        ResourceType::CBuffer,
        ResourceType::RWTexture2D
    }, true);

    // Create the pipeline
    ShaderBlob blob = ShaderHost::Get("TextureIndexOOBTest", device->GetName());
    PipelineID pipeline = device->CreateComputePipeline(&layout, 1u, blob.code, blob.length);

    // Contant buffer data
    struct CBuffer {
        uint32_t gCBOffset = 4000000;
    } cbuffer;

    // Create sets
    ResourceSetID resourceSet = device->CreateResourceSet(layout, {
        device->CreateCBuffer(64, &cbuffer, sizeof(cbuffer)),
        device->CreateTexture(ResourceType::RWTexture2D, Backend::IL::Format::R32Float, 64u, 64u, 1u, nullptr, 0)
    });

    // Create command buffer
    CommandBufferID commandBuffer = device->CreateCommandBuffer(QueueType::Graphics);

    // Begin!
    device->BeginCommandBuffer(commandBuffer);
    device->InitializeResources(commandBuffer);

    // Bind pipeline and resources
    device->BindPipeline(commandBuffer, pipeline);
    device->BindResourceSet(commandBuffer, 0u, resourceSet);
    device->Dispatch(commandBuffer, 1u, 1u, 1u);

    // End!
    device->EndCommandBuffer(commandBuffer);

    // Submit on generic graphics
    device->Submit(device->GetQueue(QueueType::Graphics), commandBuffer);
}
