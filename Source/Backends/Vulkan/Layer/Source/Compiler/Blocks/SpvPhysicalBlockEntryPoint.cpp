#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockEntryPoint.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Compiler/SpvParseContext.h>

void SpvPhysicalBlockEntryPoint::Parse() {
    block = table.scan.GetPhysicalBlock(SpvPhysicalBlockType::EntryPoint);

    // Parse instructions
    SpvParseContext ctx(block->source);
    while (ctx) {
        switch (ctx->GetOp()) {
            default:
                break;
            case SpvOpEntryPoint:
                ctx++;

                program.SetEntryPoint(ctx++);
                break;
        }

        // Next instruction
        ctx.Next();
    }
}
