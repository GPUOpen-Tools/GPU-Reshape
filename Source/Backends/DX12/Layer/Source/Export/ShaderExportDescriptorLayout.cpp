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

    // Shared PRMT buffer
    prmtOffset = descriptorOffset;
    descriptorOffset += descriptorStride;

    // Number of resources
    uint32_t resourceCount;
    device->shaderDataHost->Enumerate(&resourceCount, nullptr, ShaderDataType::DescriptorMask);

    // Shader Datas, each take a slot
    shaderDataOffset = descriptorOffset;
    descriptorOffset += descriptorStride * resourceCount;
}
