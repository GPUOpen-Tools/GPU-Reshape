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

// Backend
#include <Backend/IL/Metadata/KernelMetadata.h>

void SpvPhysicalBlockEntryPoint::Parse() {
    block = table.scan.GetPhysicalBlock(SpvPhysicalBlockType::EntryPoint);

    // Parse instructions
    SpvParseContext ctx(block->source);
    while (ctx) {
        switch (ctx->GetOp()) {
            default:
                break;
            case SpvOpEntryPoint: {
                // Create entrypoint entry
                EntryPoint& entryPoint = entryPoints.emplace_back();
                entryPoint.executionModel = static_cast<SpvExecutionModel>(ctx++);
                entryPoint.id = ctx++;
                program.SetEntryPoint(entryPoint.id);

                // Parse name
                auto* ptr = reinterpret_cast<const char*>(ctx.GetInstructionCode());
                size_t length = std::strlen(ptr);
                ctx.Skip(static_cast<uint32_t>((length + 3) / 4));
                entryPoint.name = ptr;

                // Parse all interfaces
                while (ctx.HasPendingWords()) {
                    entryPoint.interfaces.push_back(ctx++);
                }
                break;
            }
            case SpvOpExecutionMode:
            case SpvOpExecutionModeId: {
                EntryPoint* entryPoint = GetEntryPoint(ctx++);
                ASSERT(entryPoint, "Invalid entrypoint");

                // Create mode entry
                ExecutionMode& executionMode = entryPoint->executionModes.emplace_back();
                executionMode.opCode = ctx->GetOp();
                executionMode.executionMode = static_cast<SpvExecutionMode>(ctx++);

                // Read all payload words
                while (ctx.HasPendingWords()) {
                    executionMode.payload.Add(ctx++);
                }

                // Create program metadata
                switch (executionMode.executionMode) {
                    default: {
                        break;
                    }
                    case SpvExecutionModeLocalSize: {
                        program.GetMetadataMap().AddMetadata(entryPoint->id, IL::KernelWorkgroupSizeMetadata {
                            .threadsX = executionMode.payload[0],
                            .threadsY = executionMode.payload[1],
                            .threadsZ = executionMode.payload[2]
                        });
                        break;
                    }
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

    // Recompile all entry points
    for (EntryPoint& entryPoint : entryPoints) {
        // Compile the SpvOpEntryPoint
        CompileEntryPoint(entryPoint);
    }
    
    // Compile all execution modes
    // Must come after the entry points
    for (EntryPoint& entryPoint : entryPoints) {
        for (ExecutionMode& executionMode : entryPoint.executionModes) {
            CompileExecutionMode(entryPoint.id, executionMode);
        }
    }
}

void SpvPhysicalBlockEntryPoint::CompileEntryPoint(const EntryPoint &entryPoint) {
    // Total number of dwords for the name
    uint32_t nameDWordCount = static_cast<uint32_t>((entryPoint.name.length() + 3) / 4);

    // Emit instruction
    SpvInstruction& instr = block->stream.Allocate(SpvOpEntryPoint, static_cast<uint32_t>(3 + nameDWordCount + entryPoint.interfaces.size()));
    instr[1] = static_cast<uint32_t>(entryPoint.executionModel);
    instr[2] = entryPoint.id;
    std::memset(&instr[3], 0x0, sizeof(uint32_t) * nameDWordCount);

    // Write name
    for (size_t i = 0; i < entryPoint.name.size(); i++) {
        instr[static_cast<uint32_t>(3 + (i / 4))] |= entryPoint.name[i] << (i % 4) * 8;
    }

    // Write all interfaces
    for (size_t i = 0; i < entryPoint.interfaces.size(); i++) {
        instr[static_cast<uint32_t>(3 + nameDWordCount + i)] = entryPoint.interfaces[i];
    }
}

void SpvPhysicalBlockEntryPoint::CompileExecutionMode(SpvId entryPoint, ExecutionMode &executionMode) {
    // Metadata overrides
    switch (executionMode.executionMode) {
        default: {
            break;
        }
        case SpvExecutionModeLocalSize: {
            auto workgroupSize = program.GetMetadataMap().GetMetadata<IL::KernelWorkgroupSizeMetadata>(entryPoint);
            executionMode.payload[0] = workgroupSize->threadsX;
            executionMode.payload[1] = workgroupSize->threadsY;
            executionMode.payload[2] = workgroupSize->threadsZ;
            break;
        }
    }

    // Emit mode
    SpvInstruction& instr = block->stream.Allocate(executionMode.opCode, static_cast<uint32_t>(3 + executionMode.payload.Size()));
    instr[1] = entryPoint;
    instr[2] = executionMode.executionMode;
    std::memcpy(&instr[3], executionMode.payload.Data(), sizeof(uint32_t) * executionMode.payload.Size());
}

SpvPhysicalBlockEntryPoint::EntryPoint * SpvPhysicalBlockEntryPoint::GetEntryPoint(SpvId id) {
    for (EntryPoint& entryPoint : entryPoints) {
        if (entryPoint.id == id) {
            return &entryPoint;
        }
    }

    // Not found
    return nullptr;
}

void SpvPhysicalBlockEntryPoint::CopyTo(SpvPhysicalBlockTable &remote, SpvPhysicalBlockEntryPoint &out) {
    out.block = remote.scan.GetPhysicalBlock(SpvPhysicalBlockType::EntryPoint);
    out.entryPoints = entryPoints;
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
