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
            case SpvOpEntryPoint: {
                executionModel = static_cast<SpvExecutionModel>(ctx++);
                program.SetEntryPoint(ctx++);

                // Parse name
                auto* ptr = reinterpret_cast<const char*>(ctx.GetInstructionCode());
                size_t length = std::strlen(ptr);
                ctx.Skip(static_cast<uint32_t>((length + 3) / 4));
                name = ptr;

                // Parse all interfaces
                while (ctx.HasPendingWords()) {
                    interfaces.push_back(ctx++);
                }
                break;
            }
        }

        // Next instruction
        ctx.Next();
    }
}

void SpvPhysicalBlockEntryPoint::Compile() {
    block->stream.Clear();

    // Total number of dwords for the name
    uint32_t nameDWordCount = static_cast<uint32_t>((name.length() + 3) / 4);

    // Emit instruction
    SpvInstruction& instr = block->stream.Allocate(SpvOpEntryPoint, static_cast<uint32_t>(3 + nameDWordCount + interfaces.size()));
    instr[1] = static_cast<uint32_t>(executionModel);
    instr[2] = program.GetEntryPoint()->GetID();
    std::memset(&instr[3], 0x0, sizeof(uint32_t) * nameDWordCount);

    // Write name
    for (size_t i = 0; i < name.size(); i++) {
        instr[static_cast<uint32_t>(3 + (i / 4))] |= name[i] << (i % 4) * 8;
    }

    // Write all interfaces
    for (size_t i = 0; i < interfaces.size(); i++) {
        instr[static_cast<uint32_t>(3 + nameDWordCount + i)] = interfaces[i];
    }
}

void SpvPhysicalBlockEntryPoint::CopyTo(SpvPhysicalBlockTable &remote, SpvPhysicalBlockEntryPoint &out) {
    out.block = remote.scan.GetPhysicalBlock(SpvPhysicalBlockType::EntryPoint);
    out.executionModel = executionModel;
    out.name = name;
    out.interfaces = interfaces;
}

void SpvPhysicalBlockEntryPoint::AddInterface(SpvStorageClass storageClass, SpvId id) {
    // If we're not on 1.4, only report Input | Output storage classes
    if (!table.scan.VersionSatisfies(1, 4)) {
        if (storageClass != SpvStorageClassInput && storageClass != SpvStorageClassOutput) {
            return;
        }
    }

    // Passed, add it!
    AddInterface(id);
}
