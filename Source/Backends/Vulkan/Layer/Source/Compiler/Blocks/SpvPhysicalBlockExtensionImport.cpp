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

#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockExtensionImport.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Compiler/SpvParseContext.h>

void SpvPhysicalBlockExtensionImport::Parse() {
    block = table.scan.GetPhysicalBlock(SpvPhysicalBlockType::ExtensionImport);

    // Parse instructions
    SpvParseContext ctx(block->source);
    while (ctx) {
        switch (ctx->GetOp()) {
            default: {
                break;
            }
            case SpvOpExtInstImport: {
                sets.push_back(InstructionSet {
                    .name = reinterpret_cast<const char*>(ctx.GetInstructionCode()),
                    .id = ctx.GetResult()
                });
                break;
            }
        }

        // Next instruction
        ctx.Next();
    }
}

IL::ID SpvPhysicalBlockExtensionImport::Get(ShortHashString name) {
    for (const InstructionSet& set : sets) {
        if (set.name.hash == name.hash) {
            return set.id;
        }
    }
    
    return IL::InvalidID;
}

IL::ID SpvPhysicalBlockExtensionImport::GetOrAdd(ShortHashString name) {
    if (IL::ID id = Get(name); id != IL::InvalidID) {
        return id;
    }

    // Keep track of it
    IL::ID id = table.scan.header.bound++;
    sets.push_back(InstructionSet {
        .name = name,
        .id = id
    });

    // Total number of dwords for the name
    uint32_t length = static_cast<uint32_t>(std::strlen(name.name));
    uint32_t nameDWordCount = (length + /* Null Terminator*/ 1 + 3) / 4;

    // Allocate import
    SpvInstruction& instr = block->stream.Allocate(SpvOpExtInstImport, 2 + nameDWordCount);
    instr[1] = id;
    std::memset(&instr[2], 0x0, sizeof(uint32_t) * nameDWordCount);

    // Write name
    for (size_t i = 0; i < length; i++) {
        instr[static_cast<uint32_t>(2 + (i / 4))] |= name.name[i] << (i % 4) * 8;
    }

    return id;
}

void SpvPhysicalBlockExtensionImport::CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockExtensionImport &out) {
    out.block = remote.scan.GetPhysicalBlock(SpvPhysicalBlockType::ExtensionImport);
    out.sets = sets;
}
