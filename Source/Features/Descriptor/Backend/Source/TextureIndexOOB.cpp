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
        device->CreateTexture(ResourceType::RWTexture2D, IL::Format::R32Float, 64u, 64u, 1u, nullptr, 0)
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
