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

        // Set bound
        registerBound = std::max(registerBound, entry.scan._register + 1);
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

    // Fill string-buffer
    std::vector<char> stringBuffer;

    // Entries right after the header
    header.offset = sizeof(DXILInputSignature);

    // Append header and strings
    block->stream.Append(header);

    // Start of string buffer
    uint32_t stringStart = block->stream.GetOffset() + static_cast<uint32_t>(sizeof(DXILSignatureElement) * entries.size());

    // Append all entries
    for (ElementEntry& entry : entries) {
        // Set strings
        entry.scan.semanticNameOffset = stringStart + static_cast<uint32_t>(stringBuffer.size());
        stringBuffer.insert(stringBuffer.end(), entry.semanticName.begin(), entry.semanticName.end());
        stringBuffer.push_back('\0');

        // Append entry
        block->stream.Append(entry.scan);
    }

    // Append strings
    block->stream.AppendData(stringBuffer.data(), static_cast<uint32_t>(stringBuffer.size()));
    block->stream.AlignTo(4);
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

uint32_t DXBCPhysicalBlockInputSignature::AddOrGetInput(const std::string &name, DXILSignatureElementSemantic semantic, DXILSignatureElementComponentType type, IL::ComponentMaskSet mask, DXILSignatureElementPrecision precision) {
    for (ElementEntry &entry : entries) {
        if (entry.semanticName == name) {
            return entry.scan._register;
        }
    }

    // Write out entry
    ElementEntry entry{};
    entry.scan.streamIndex = 0;
    entry.scan.semanticIndex = 0;
    entry.scan.semantic = semantic;
    entry.scan.componentType = type;
    entry.scan.mask = mask.value;
    entry.scan._register = registerBound++;
    entry.scan.writeMask = 0x0;
    entry.scan.precision = precision;
    entry.scan.pad = 0;
    entries.push_back(entry);
    
    return entry.scan._register;
}
