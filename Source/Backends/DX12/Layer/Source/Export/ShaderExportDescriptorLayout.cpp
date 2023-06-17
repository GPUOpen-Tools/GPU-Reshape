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

#include <Backends/DX12/Export/ShaderExportDescriptorLayout.h>
#include <Backends/DX12/ShaderData/ShaderDataHost.h>
#include <Backends/DX12/Export/ShaderExportHost.h>
#include <Backends/DX12/States/DeviceState.h>

void ShaderExportDescriptorLayout::Install(DeviceState *device, uint32_t stride) {
    descriptorStride = stride;

    // Number of exports
    uint32_t exportCount;
    device->exportHost->Enumerate(&exportCount, nullptr);

    // Export counters
    shaderExportCounterOffset = descriptorOffset;
    descriptorOffset += descriptorStride;

    // Export streams, each take a slot
    shaderExportStreamOffset = descriptorOffset;
    descriptorOffset += exportCount * descriptorStride;

    // Shared SRV PRMT buffer
    resourcePRMTOffset = descriptorOffset;
    descriptorOffset += descriptorStride;

    // Shared sampler PRMT buffer
    samplerPRMTOffset = descriptorOffset;
    descriptorOffset += descriptorStride;

    // Shader constant data cbv
    shaderConstantOffset = descriptorOffset;
    descriptorOffset += descriptorStride;

    // Number of resources
    uint32_t resourceCount;
    device->shaderDataHost->Enumerate(&resourceCount, nullptr, ShaderDataType::DescriptorMask);

    // Shader Datas, each take a slot
    shaderDataOffset = descriptorOffset;
    descriptorOffset += descriptorStride * resourceCount;
}
