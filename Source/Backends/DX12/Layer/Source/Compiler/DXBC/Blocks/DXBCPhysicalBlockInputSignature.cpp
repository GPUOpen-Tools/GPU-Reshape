#include <Backends/DX12/Compiler/DXBC/Blocks/DXBCPhysicalBlockInputSignature.h>
#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXBC/DXBCParseContext.h>
#include <Backends/DX12/Compiler/DXIL/DXILModule.h>

// Common
#include <Common/Sink.h>

DXBCPhysicalBlockInputSignature::DXBCPhysicalBlockInputSignature(const Allocators &allocators, Backend::IL::Program &program, DXBCPhysicalBlockTable &table)
    : DXBCPhysicalBlockSection(allocators, program, table), entries(allocators) {
    /* */
}

void DXBCPhysicalBlockInputSignature::Parse() {
    // Block is optional
    DXBCPhysicalBlock *block = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::InputSignature);
    if (!block) {
        return;
    }

    // Setup parser
    DXBCParseContext ctx(block->ptr, block->length);

    // Get the header
    header = ctx.Consume<DXILInputSignature>();

    // Get parameter start
    const auto *parameterStart = reinterpret_cast<const DXILSignatureElement *>(block->ptr + header.offset);

    // Read all parameters
    for (uint32_t i = 0; i < header.count; i++) {
        const DXILSignatureElement &source = parameterStart[i];

        // Create entries
        ElementEntry& entry = entries.emplace_back();
        entry.scan = source;
        entry.semanticName = reinterpret_cast<const char*>(block->ptr + source.semanticNameOffset);
    }
}

void DXBCPhysicalBlockInputSignature::Compile() {
    // Block is optional
    DXBCPhysicalBlock *block = table.scan.GetPhysicalBlock(DXBCPhysicalBlockType::InputSignature);
    if (!block) {
        return;
    }

    // Get compiled binding info
    ASSERT(table.dxilModule, "IS not supported for native DXBC");
    const RootRegisterBindingInfo& bindingInfo = table.dxilModule->GetBindingInfo().bindingInfo;

    // Not used yet
    GRS_SINK(bindingInfo);
}

void DXBCPhysicalBlockInputSignature::CopyTo(DXBCPhysicalBlockInputSignature &out) {
    out.header = header;
    out.entries = entries;
}
