#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockCapability.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Compiler/SpvParseContext.h>

void SpvPhysicalBlockCapability::Parse() {
    block = table.scan.GetPhysicalBlock(SpvPhysicalBlockType::Capability);

    // Parse instructions
    SpvParseContext ctx(block->source);
    while (ctx) {
        switch (ctx->GetOp()) {
            default:
                break;
            case SpvOpCapability:
                capabilities.insert(static_cast<SpvCapability>(ctx++));
                break;
        }

        // Next instruction
        ctx.Next();
    }
}

void SpvPhysicalBlockCapability::Add(SpvCapability capability) {
    if (capabilities.count(capability)) {
        return;
    }

    // Allocate instruction
    SpvInstruction& instr = block->stream.Allocate(SpvOpCapability, 2);
    instr[1] = capability;
    capabilities.insert(capability);
}

void SpvPhysicalBlockCapability::CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockCapability &out) {
    out.block = remote.scan.GetPhysicalBlock(SpvPhysicalBlockType::Capability);
    out.capabilities = capabilities;
}
