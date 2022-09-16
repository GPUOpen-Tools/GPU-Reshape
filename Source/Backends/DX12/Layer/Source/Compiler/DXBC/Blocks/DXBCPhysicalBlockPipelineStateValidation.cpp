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

    // Read version 0
    if (runtimeInfoSize >= sizeof(DXBCPSVRuntimeInfoRevision1)) {
        runtimeInfo.info1 = ctx.Consume<DXBCPSVRuntimeInfo1>();
    }

    // Read version 0
    if (runtimeInfoSize >= sizeof(DXBCPSVRuntimeInfoRevision2)) {
        runtimeInfo.info2 = ctx.Consume<DXBCPSVRuntimeInfo2>();
    }
}

void DXBCPhysicalBlockPipelineStateValidation::Compile() {
    // Block is optional
    DXBCPhysicalBlock* block = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::PipelineStateValidation);
    if (!block) {
        return;
    }

    // Setup parser
    DXBCParseContext ctx(block->ptr, block->length);
    ctx.Skip(sizeof(uint32_t));
    ctx.Skip(runtimeInfoSize);

    // Emit runtime info with the original size
    block->stream.Append(runtimeInfoSize);
    block->stream.AppendData(&runtimeInfo, runtimeInfoSize);

    // Number of existing resources
    auto resourceCount = ctx.Consume<uint32_t>();
    block->stream.Append(resourceCount + 1);

    // Size of the bind info
    uint32_t bindInfoSize;

    // Any?
    if (resourceCount) {
        bindInfoSize = ctx.Consume<uint32_t>();
    } else {
        if (runtimeInfoSize >= sizeof(DXBCPSVRuntimeInfoRevision2)) {
            bindInfoSize = sizeof(DXBCPSVBindInfoRevision1);
        } else {
            bindInfoSize = sizeof(DXBCPSVBindInfoRevision0);
        }
    }

    // Set size
    block->stream.Append(bindInfoSize);

    // Skip current resources
    block->stream.AppendData(ctx.ptr, bindInfoSize * resourceCount);

    // Skip existing binds
    for (uint32_t i = 0; i < resourceCount; i++) {
        auto& data = ctx.Get<DXBCPSVBindInfoRevision1>();
        ctx.Skip(bindInfoSize);
    }

    // Get compiled binding info
    ASSERT(table.dxilModule, "PSV not supported for native DXBC");
    const DXILBindingInfo& bindingInfo = table.dxilModule->GetBindingInfo();

    // Write base bind info
    block->stream.Append(DXBCPSVBindInfo0{
        .type = DXBCPSVBindInfoType::UnorderedAccessView,
        .space = bindingInfo.space,
        .low = bindingInfo._register,
        .high = bindingInfo._register + (bindingInfo.count - 1)
    });

    // Extended1?
    if (bindInfoSize >= sizeof(DXBCPSVBindInfoRevision1)) {
        block->stream.Append(DXBCPSVBindInfo1{
            .kind = DXBCPSVBindInfoKind::TypedBuffer,
            .flags = 0
        });
    }

    // Add post
    block->stream.AppendData(ctx.ptr, block->length - ctx.Offset());
}

void DXBCPhysicalBlockPipelineStateValidation::CopyTo(DXBCPhysicalBlockPipelineStateValidation &out) {
    out.runtimeInfoSize = runtimeInfoSize;
    out.runtimeInfo = runtimeInfo;
}
