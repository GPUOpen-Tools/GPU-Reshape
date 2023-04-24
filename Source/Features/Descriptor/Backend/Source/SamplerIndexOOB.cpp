#include <Test/Device/IDevice.h>
#include <Test/Device/ShaderHost.h>

void SamplerIndexOOBExecutor(Test::IDevice *device) {
    using namespace Test;

    ResourceLayoutID layouts[] = {
        device->CreateResourceLayout({
            ResourceType::CBuffer,
            ResourceType::RWTexelBuffer,
            ResourceType::Texture2D
        }, true),
        device->CreateResourceLayout({
            ResourceType::SamplerState
        }, true)
    };

    // Create the pipeline
    ShaderBlob blob = ShaderHost::Get("SamplerIndexOOBTest", device->GetName());
    PipelineID pipeline = device->CreateComputePipeline(layouts, 2u, blob.code, blob.length);

    // Contant buffer data
    struct CBuffer {
        uint32_t gCBOffset = 4000000;
    } cbuffer;

    // Create sets
    ResourceSetID resourceSets[] = {
        device->CreateResourceSet(layouts[0], {
            device->CreateCBuffer(64, &cbuffer, sizeof(cbuffer)),
            device->CreateTexelBuffer(ResourceType::RWTexelBuffer, Backend::IL::Format::R32Float, 64u, nullptr, 0),
            device->CreateTexture(ResourceType::Texture2D, Backend::IL::Format::R32Float, 64u, 64u, 1u, nullptr, 0)
        }),
        device->CreateResourceSet(layouts[1], {
            device->CreateSampler()
        })
    };

    // Create command buffer
    CommandBufferID commandBuffer = device->CreateCommandBuffer(QueueType::Graphics);

    // Begin!
    device->BeginCommandBuffer(commandBuffer);
    device->InitializeResources(commandBuffer);

    // Bind pipeline and resources
    device->BindPipeline(commandBuffer, pipeline);
    device->BindResourceSet(commandBuffer, 0u, resourceSets[0]);
    device->BindResourceSet(commandBuffer, 1u, resourceSets[1]);
    device->Dispatch(commandBuffer, 1u, 1u, 1u);

    // End!
    device->EndCommandBuffer(commandBuffer);

    // Submit on generic graphics
    device->Submit(device->GetQueue(QueueType::Graphics), commandBuffer);
}
