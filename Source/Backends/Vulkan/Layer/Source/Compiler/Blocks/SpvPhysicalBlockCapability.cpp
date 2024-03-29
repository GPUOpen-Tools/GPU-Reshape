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
