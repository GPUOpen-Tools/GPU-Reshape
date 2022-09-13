#include <Backends/DX12/Compiler/DXBC/Blocks/DXBCPhysicalBlockShader.h>
#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXBC/DXBCParseContext.h>

void DXBCPhysicalBlockShader::Parse() {
    // Block is optional
    DXBCPhysicalBlock* block = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::Shader5);
    if (!block) {
        return;
    }

    // Setup parser
    DXBCParseContext ctx(block->ptr, block->length);

    // Consume header
    auto header = ctx.Consume<DXBCShaderHeader>();

    // ...
    while (ctx.IsGood()) {
        return;
    }
}
