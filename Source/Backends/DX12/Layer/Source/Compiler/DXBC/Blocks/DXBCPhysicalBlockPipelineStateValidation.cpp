#include <Backends/DX12/Compiler/DXBC/Blocks/DXBCPhysicalBlockPipelineStateValidation.h>
#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXBC/DXBCParseContext.h>

void DXBCPhysicalBlockPipelineStateValidation::Parse() {
    // Block is optional
    DXBCPhysicalBlock* block = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::PipelineStateValidation);
    if (!block) {
        return;
    }

    // Setup parser
    DXBCParseContext ctx(block->ptr, block->length);

    // Skip runtime info
    auto runtimeInfoSize = ctx.Consume<uint32_t>();
    ctx.Skip(runtimeInfoSize);

    // Resource offset
    resourceOffset = ctx.Offset();
}

void DXBCPhysicalBlockPipelineStateValidation::Compile() {
    // Block is optional
    DXBCPhysicalBlock* block = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::PipelineStateValidation);
    if (!block) {
        return;
    }

    // Add pre
    block->stream.AppendData(block->ptr, resourceOffset);

    // Setup parser
    DXBCParseContext ctx(block->ptr, block->length);
    ctx.Skip(resourceOffset);

    // Number of existing resources
    auto resourceCount = ctx.Consume<uint32_t>();
    block->stream.Append(resourceCount + 1);

    // Size of the bind info
    uint32_t bindInfoSize;

    // Any?
    if (resourceCount) {
        bindInfoSize = ctx.Consume<uint32_t>();
    } else {
        bindInfoSize = sizeof(DXBCPSVBindInfoRevision1);
    }

    // Set size
    block->stream.Append(bindInfoSize);

    // Skip current resources
    block->stream.AppendData(ctx.ptr, bindInfoSize * resourceCount);
    ctx.Skip(bindInfoSize * resourceCount);

    // Write base bind info
    block->stream.Append(DXBCPSVBindInfo0{
        .type = DXBCPSVBindInfoType::UnorderedAccessView,
        .space = 0,
        .low = 1,
        .high = 1
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
    out.resourceOffset = resourceOffset;
}
