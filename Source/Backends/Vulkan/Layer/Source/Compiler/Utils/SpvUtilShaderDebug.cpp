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

#include <Backends/Vulkan/Compiler/Utils/SpvUtilShaderDebug.h>
#include <Backends/Vulkan/Compiler/SpvPhysicalBlockTable.h>
#include <Backends/Vulkan/Compiler/SpvParseContext.h>
#include <Backends/Vulkan/Compiler/SpvRecordReader.h>
#include <Backends/Vulkan/Compiler/SpvJob.h>

// SPIRV
#include <spirv/unified1/NonSemanticShaderDebugInfo100.h>

// Common
#include <Common/Containers/TrivialStackVector.h>

SpvUtilShaderDebug::SpvUtilShaderDebug(const Allocators &allocators, IL::Program &program, SpvPhysicalBlockTable &table) :
    allocators(allocators),
    program(program),
    table(table),
    sourceMap(allocators) {

}

void SpvUtilShaderDebug::Parse() {
    // Set entry count
    debugMap.SetBound(table.scan.header.bound);

    // Query the extension, may return an invalid id
    static ShortHashString debugInfo100Name("NonSemantic.Shader.DebugInfo.100");
    extDebugInfo100 = table.extensionImport.Get(debugInfo100Name);
}

void SpvUtilShaderDebug::FinalizeSource() {
    // Finalize all sources
    sourceMap.Finalize();
}

void SpvUtilShaderDebug::ParseInstruction(SpvParseContext &ctx) {
    switch (ctx->GetOp()) {
        default:
            break;
        case SpvOpSource: {
            auto language = static_cast<SpvSourceLanguage>(ctx++);
            uint32_t version = ctx++;

            // Optional filename
            uint32_t fileId{InvalidSpvId};
            if (ctx.HasPendingWords()) {
                fileId = ctx++;
                sourceMap.AddPhysicalSource(fileId, language, version, debugMap.Get(fileId, SpvOpString));

                // Set pending
                pendingSource = fileId;
            }

            // Optional fragment?
            if (ctx.HasPendingWords()) {
                auto* ptr = reinterpret_cast<const char*>(ctx.GetInstructionCode());
                uint32_t length = (ctx->GetWordCount() - 4) * sizeof(uint32_t);
                while (length && !ptr[length - 1]) {
                    length--;
                }

                sourceMap.AddSource(fileId, std::string_view(ptr, length));
            }
            break;
        }
        case SpvOpSourceContinued: {
            auto* ptr = reinterpret_cast<const char*>(ctx.GetInstructionCode());
            uint32_t length = (ctx->GetWordCount() - 1) * sizeof(uint32_t);
            while (length && !ptr[length - 1]) {
                length--;
            }

            sourceMap.AddSource(pendingSource, std::string_view(ptr, length));
            break;
        }
        case SpvOpString: {
            auto* ptr = reinterpret_cast<const char*>(ctx.GetInstructionCode());
            uint32_t length = (ctx->GetWordCount() - 2) * sizeof(uint32_t);
            while (length && !ptr[length - 1]) {
                length--;
            }

            debugMap.Add(ctx.GetResult(), SpvOpString, std::string_view(ptr, length));
            break;
        }
    }
}

void SpvUtilShaderDebug::ParseDebug100Instruction(SpvRecordReader &ctx) {
    ENSURE(ctx++ == extDebugInfo100, "Unexpected set index");

    // Handle instruction
    switch (ctx++) {
        default: {
            break;
        }
        case NonSemanticShaderDebugInfo100DebugCompilationUnit: {
            debug100CompilationUnit.version = table.typeConstantVariable.GetConstantLiteral(ctx++);
            debug100CompilationUnit.dwarfVersion = table.typeConstantVariable.GetConstantLiteral(ctx++);
            debug100CompilationUnit.source100Id = ctx++;
            debug100CompilationUnit.language = table.typeConstantVariable.GetConstantLiteral<SpvSourceLanguage>(ctx++);
            break;
        }
        case NonSemanticShaderDebugInfo100DebugSource: {
            SpvId fileId = ctx++;

            // Add the source
            uint32_t fileIndex = sourceMap.AddPhysicalSource(fileId, SpvSourceLanguageUnknown, 0, debugMap.Get(fileId, SpvOpString));

            // Set pending
            pendingSource = fileId;

            // Contents is optional
            if (ctx.HasPendingWords()) {
                sourceMap.AddSource(fileId, debugMap.Get(ctx++, SpvOpString));
            }

            // Set metadata
            Debug100Metadata& metadata = GetDebug100Metadata(ctx.GetResult());
            metadata.debugSource.fileIndex = fileIndex;
            break;
        }
        case NonSemanticShaderDebugInfo100DebugSourceContinued: {
            // Just append from last pending
            sourceMap.AddSource(pendingSource, debugMap.Get(ctx++, SpvOpString));
            break;
        }
    }
}

void SpvUtilShaderDebug::ParseDebug100FunctionInstruction(SpvParseContext &ctx, SpvSourceAssociation& sourceAssociation) {
    ENSURE(ctx++ == extDebugInfo100, "Unexpected set index");

    // Handle instruction
    switch (ctx++) {
        default: {
            break;
        }
        case NonSemanticShaderDebugInfo100DebugLine: {
            sourceAssociation.fileUID = debug100Metadata.at(ctx++).debugSource.fileIndex;

            // Parse line, ignore end
            sourceAssociation.line = table.typeConstantVariable.GetConstantLiteral(ctx++) - 1;
            ctx++;
            
            // Parse column, ignore end
            sourceAssociation.column = table.typeConstantVariable.GetConstantLiteral(ctx++) - 1;
            ctx++;

            if (sourceAssociation.column == UINT16_MAX) {
                sourceAssociation.column = 0;
            }
            break;
        }

        case NonSemanticShaderDebugInfo100DebugNoLine: {
            sourceAssociation = {};
            break;
        }
    }
}

void SpvUtilShaderDebug::CopyTo(SpvPhysicalBlockTable &remote, SpvUtilShaderDebug &out) {
    out.debugMap = debugMap;
    out.sourceMap = sourceMap;
}

SpvUtilShaderDebug::Debug100Metadata & SpvUtilShaderDebug::GetDebug100Metadata(SpvId id) {
    if (id >= debug100Metadata.size()) {
        debug100Metadata.resize(id + 1);
    }

    return debug100Metadata[id];
}
