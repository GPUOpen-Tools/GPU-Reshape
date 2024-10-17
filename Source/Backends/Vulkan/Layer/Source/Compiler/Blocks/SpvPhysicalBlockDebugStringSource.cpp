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

#include <Backends/Vulkan/Compiler/Blocks/SpvPhysicalBlockDebugStringSource.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Compiler/SpvParseContext.h>

SpvPhysicalBlockDebugStringSource::SpvPhysicalBlockDebugStringSource(const Allocators &allocators, Backend::IL::Program &program, SpvPhysicalBlockTable &table)
    : SpvPhysicalBlockSection(allocators, program, table) {
    
}

void SpvPhysicalBlockDebugStringSource::Parse() {
    block = table.scan.GetPhysicalBlock(SpvPhysicalBlockType::DebugStringSource);
    
    // Parse instructions
    SpvParseContext ctx(block->source);
    while (ctx) {
        switch (ctx->GetOp()) {
            default:
                break;
            case SpvOpSource:
            case SpvOpSourceContinued:
            case SpvOpString: {
                table.shaderDebug.ParseInstruction(ctx);
                break;
            }
        }

        // Next instruction
        ctx.Next();
    }
}

void SpvPhysicalBlockDebugStringSource::CopyTo(SpvPhysicalBlockTable& remote, SpvPhysicalBlockDebugStringSource &out) {
    out.block = remote.scan.GetPhysicalBlock(SpvPhysicalBlockType::DebugStringSource);
}
