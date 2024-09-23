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

#include <Backends/DX12/Compiler/DXBC/Blocks/DXBCPhysicalBlockInputSignature.h>
#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockTable.h>
#include <Backends/DX12/Compiler/DXBC/DXBCParseContext.h>
#include <Backends/DX12/Compiler/DXIL/DXILModule.h>
#include <Backends/DX12/Compiler/DXCompileJob.h>

// Common
#include <Common/Sink.h>

DXBCPhysicalBlockInputSignature::DXBCPhysicalBlockInputSignature(const Allocators &allocators, IL::Program &program, DXBCPhysicalBlockTable &table)
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

void DXBCPhysicalBlockInputSignature::CompileDXILCompatability(DXCompileJob &job) {
    for (const ElementEntry& entry : entries) {
        if (entry.scan.semantic == DXILSignatureElementSemantic::ViewPortArrayIndex) {
            job.compatabilityTable.useViewportAndRTArray = true;
        }
    }
}

void DXBCPhysicalBlockInputSignature::CopyTo(DXBCPhysicalBlockInputSignature &out) {
    out.header = header;
    out.entries = entries;
}
