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

#include <Backends/DX12/Compiler/DXBC/Blocks/DXBCPhysicalBlockPipelineStateValidation.h>
#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXBC/DXBCParseContext.h>
#include <Backends/DX12/Compiler/DXIL/DXILModule.h>

void DXBCPhysicalBlockPipelineStateValidation::Parse() {
    // Block is optional
    DXBCPhysicalBlock* block = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::PipelineStateValidation);
    if (!block) {
        return;
    }

    // Setup parser
    DXBCParseContext ctx(block->ptr, block->length);

    // Get the size
    runtimeInfoSize = ctx.Consume<uint32_t>();

    // Read version 0
    if (runtimeInfoSize >= sizeof(DXBCPSVRuntimeInfoRevision0)) {
        runtimeInfo.info0 = ctx.Consume<DXBCPSVRuntimeInfo0>();
    }

    // Read version 1
    if (runtimeInfoSize >= sizeof(DXBCPSVRuntimeInfoRevision1)) {
        runtimeInfo.info1 = ctx.Consume<DXBCPSVRuntimeInfo1>();
    }

    // Read version 2
    if (runtimeInfoSize >= sizeof(DXBCPSVRuntimeInfoRevision2)) {
        runtimeInfo.info2 = ctx.Consume<DXBCPSVRuntimeInfo2>();
    }

    // Read version 3
    if (runtimeInfoSize >= sizeof(DXBCPSVRuntimeInfoRevision3)) {
        runtimeInfo.info3 = ctx.Consume<DXBCPSVRuntimeInfo3>();
    }

    // Number of existing resources
    auto resourceCount = ctx.Consume<uint32_t>();
    if (!resourceCount) {
        if (runtimeInfoSize >= sizeof(DXBCPSVRuntimeInfoRevision2)) {
            runtimeBindingsSize = sizeof(DXBCPSVBindInfoRevision1);
        } else {
            runtimeBindingsSize = sizeof(DXBCPSVBindInfoRevision0);
        }

        // Set dangling offset
        offset = ctx.Offset();
        return;
    }

    // Size of the bind info
    runtimeBindingsSize = ctx.Consume<uint32_t>();

    // Parse all resources
    for (uint32_t i = 0; i < resourceCount; i++) {
        DXBCPSVBindInfoRevision1 bindInfo = ctx.ConsumePartial<DXBCPSVBindInfoRevision1>(runtimeBindingsSize);

        // Add to respective bucket
        switch (bindInfo.info0.type) {
            default:
                ASSERT(false, "Invalid bind info");
                break;
            case DXBCPSVBindInfoType::Sampler:
                samplers.Add(bindInfo);
                break;
            case DXBCPSVBindInfoType::CBuffer:
                cbvs.Add(bindInfo);
                break;
            case DXBCPSVBindInfoType::ShaderResourceView:
            case DXBCPSVBindInfoType::ShaderResourceViewByte:
            case DXBCPSVBindInfoType::ShaderResourceViewStructured:
                srvs.Add(bindInfo);
                break;
            case DXBCPSVBindInfoType::UnorderedAccessView:
            case DXBCPSVBindInfoType::UnorderedAccessViewByte:
            case DXBCPSVBindInfoType::UnorderedAccessViewStructured:
            case DXBCPSVBindInfoType::UnorderedAccessViewCounter:
                uavs.Add(bindInfo);
                break;
        }
    }

    // Set dangling offset
    offset = ctx.Offset();
}

void DXBCPhysicalBlockPipelineStateValidation::Compile() {
    // Block is optional
    DXBCPhysicalBlock* block = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::PipelineStateValidation);
    if (!block) {
        return;
    }

    // Get compiled binding info
    ASSERT(table.dxilModule, "PSV not supported for native DXBC");
    const RootRegisterBindingInfo& bindingInfo = table.dxilModule->GetBindingInfo().bindingInfo;

    // Shader export
    uavs.Add(DXBCPSVBindInfoRevision1 {
        .info0 = DXBCPSVBindInfo0{
            .type = DXBCPSVBindInfoType::UnorderedAccessView,
            .space = bindingInfo.space,
            .low = bindingInfo.shaderExportBaseRegister,
            .high = bindingInfo.shaderExportBaseRegister + (bindingInfo.shaderExportCount - 1u)
        },
        .info1 = DXBCPSVBindInfo1{
            .kind = DXBCPSVBindInfoKind::TypedBuffer,
            .flags = 0
        }
    });

    // Resource PRMT
    srvs.Add(DXBCPSVBindInfoRevision1 {
        .info0 = DXBCPSVBindInfo0{
            .type = DXBCPSVBindInfoType::ShaderResourceView,
            .space = bindingInfo.space,
            .low = bindingInfo.resourcePRMTBaseRegister,
            .high = bindingInfo.resourcePRMTBaseRegister
        },
        .info1 = DXBCPSVBindInfo1{
            .kind = DXBCPSVBindInfoKind::TypedBuffer,
            .flags = 0
        }
    });

    // Sampler PRMT
    srvs.Add(DXBCPSVBindInfoRevision1 {
        .info0 = DXBCPSVBindInfo0{
            .type = DXBCPSVBindInfoType::ShaderResourceView,
            .space = bindingInfo.space,
            .low = bindingInfo.samplerPRMTBaseRegister,
            .high = bindingInfo.samplerPRMTBaseRegister
        },
        .info1 = DXBCPSVBindInfo1{
            .kind = DXBCPSVBindInfoKind::TypedBuffer,
            .flags = 0
        }
    });

    // Shader constant data
    cbvs.Add(DXBCPSVBindInfoRevision1 {
        .info0 = DXBCPSVBindInfo0{
            .type = DXBCPSVBindInfoType::CBuffer,
            .space = bindingInfo.space,
            .low = bindingInfo.shaderDataConstantRegister,
            .high = bindingInfo.shaderDataConstantRegister
        },
        .info1 = DXBCPSVBindInfo1{
            .kind = DXBCPSVBindInfoKind::CBuffer,
            .flags = 0
        }
    });

    // Descriptor data
    cbvs.Add(DXBCPSVBindInfoRevision1 {
        .info0 = DXBCPSVBindInfo0{
            .type = DXBCPSVBindInfoType::CBuffer,
            .space = bindingInfo.space,
            .low = bindingInfo.descriptorConstantBaseRegister,
            .high = bindingInfo.descriptorConstantBaseRegister
        },
        .info1 = DXBCPSVBindInfo1{
            .kind = DXBCPSVBindInfoKind::CBuffer,
            .flags = 0
        }
    });

    // Event data
    cbvs.Add(DXBCPSVBindInfoRevision1 {
        .info0 = DXBCPSVBindInfo0{
            .type = DXBCPSVBindInfoType::CBuffer,
            .space = bindingInfo.space,
            .low = bindingInfo.eventConstantBaseRegister,
            .high = bindingInfo.eventConstantBaseRegister
        },
        .info1 = DXBCPSVBindInfo1{
            .kind = DXBCPSVBindInfoKind::CBuffer,
            .flags = 0
        }
    });

    // Get shader data
    IL::ShaderDataMap& shaderDataMap = table.dxilModule->GetProgram()->GetShaderDataMap();

    // Current data offset
    uint32_t dataOffset{0};

    // Create bind info per resource
    for (auto it = shaderDataMap.begin(); it != shaderDataMap.end(); it++) {
        if (!(it->type & ShaderDataType::DescriptorMask)) {
            continue;
        }

        uavs.Add(DXBCPSVBindInfoRevision1 {
            .info0 = DXBCPSVBindInfo0{
                .type = DXBCPSVBindInfoType::UnorderedAccessView,
                .space = bindingInfo.space,
                .low = bindingInfo.shaderResourceBaseRegister + dataOffset,
                .high = bindingInfo.shaderResourceBaseRegister + dataOffset
            },
            .info1 = DXBCPSVBindInfo1{
                .kind = DXBCPSVBindInfoKind::TypedBuffer,
                .flags = 0
            }
        });

        // Next
        dataOffset++;
    }

    // Emit runtime info with the original size
    block->stream.Append(runtimeInfoSize);
    block->stream.AppendData(&runtimeInfo, runtimeInfoSize);

    // Number of resources and size
    const uint32_t resourceCount = static_cast<uint32_t>(cbvs.Size() + srvs.Size() + uavs.Size() + samplers.Size());
    block->stream.Append(resourceCount);
    block->stream.Append(runtimeBindingsSize);

    // Emit CBVs
    for (const DXBCPSVBindInfoRevision1& bindInfo : cbvs) {
        block->stream.AppendData(&bindInfo, runtimeBindingsSize);
    }

    // Emit samplers
    for (const DXBCPSVBindInfoRevision1& bindInfo : samplers) {
        block->stream.AppendData(&bindInfo, runtimeBindingsSize);
    }

    // Emit SRVs
    for (const DXBCPSVBindInfoRevision1& bindInfo : srvs) {
        block->stream.AppendData(&bindInfo, runtimeBindingsSize);
    }

    // Emit UAVs
    for (const DXBCPSVBindInfoRevision1& bindInfo : uavs) {
        block->stream.AppendData(&bindInfo, runtimeBindingsSize);
    }

    // Add post
    block->stream.AppendData(block->ptr + offset, static_cast<uint32_t>(block->length - offset));
}

void DXBCPhysicalBlockPipelineStateValidation::CopyTo(DXBCPhysicalBlockPipelineStateValidation &out) {
    out.runtimeInfoSize = runtimeInfoSize;
    out.runtimeBindingsSize = runtimeBindingsSize;
    out.runtimeInfo = runtimeInfo;
    out.offset = offset;
    out.cbvs = cbvs;
    out.srvs = srvs;
    out.uavs = uavs;
    out.samplers = samplers;
}
