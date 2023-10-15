// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

#include <Backends/Vulkan/Symbolizer/ShaderSGUIDHost.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/ShaderModuleState.h>
#include <Backends/Vulkan/Compiler/SpvModule.h>
#include <Backends/Vulkan/Compiler/SpvSourceMap.h>
#include <Backends/Vulkan/Compiler/SpvCodeOffsetTraceback.h>

// Backend
#include <Backend/IL/Program.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStream.h>

// Bridge
#include <Bridge/IBridge.h>

// Schemas
#include <Schemas/SGUID.h>

ShaderSGUIDHost::ShaderSGUIDHost(DeviceDispatchTable *table) : table(table) {

}

bool ShaderSGUIDHost::Install() {
    sguidLookup.resize(1u << kShaderSGUIDBitCount);
    return true;
}

void ShaderSGUIDHost::Commit(IBridge *bridge) {
    MessageStream stream;
    MessageStreamView<ShaderSourceMappingMessage> view(stream);

    // Serial
    std::lock_guard guard(mutex);
    
    // Write all pending
    for (ShaderSGUID sguid : pendingSubmissions) {
        ShaderSourceMapping mapping = sguidLookup.at(sguid);

        // Get source
        std::string_view sourceContents = GetSource(mapping);

        // Allocate message
        ShaderSourceMappingMessage* message = view.Add(ShaderSourceMappingMessage::AllocationInfo {
            .contentsLength = sourceContents.length()
        });

        // Set SGUID
        message->sguid = sguid;

        // Fill mapping
        message->shaderGUID = mapping.shaderGUID;
        message->fileUID = mapping.fileUID;
        message->line = mapping.line;
        message->column = mapping.column;
        message->basicBlockId = mapping.basicBlockId;
        message->instructionIndex = mapping.instructionIndex;

        // Fill contents
        message->contents.Set(sourceContents);
    }

    // Export to bridge
    bridge->GetOutput()->AddStream(stream);

    // Reset
    pendingSubmissions.clear();
}

ShaderSGUID ShaderSGUIDHost::Bind(const IL::Program &program, const IL::BasicBlock::ConstIterator& instruction) {
    // Get instruction pointer
    const IL::Instruction* ptr = IL::ConstInstructionRef<>(instruction).Get();

    // Get the shader
    ShaderModuleState* shader = table->states_shaderModule.GetFromUID(program.GetShaderGUID());

    // Validate shader
    if (!shader || !shader->spirvModule) {
        return InvalidShaderSGUID;
    }

    // Get traceback
    SpvCodeOffsetTraceback traceback = shader->spirvModule->GetCodeOffsetTraceback(ptr->source.codeOffset);

    // Default mapping
    ShaderSourceMapping mapping{};
    mapping.shaderGUID = program.GetShaderGUID();

    // Mapping il association
    mapping.basicBlockId = traceback.basicBlockID;
    mapping.instructionIndex = traceback.instructionIndex;
    
    // Find the source map
    if (const SpvSourceMap* sourceMap = GetSourceMap(program.GetShaderGUID()); sourceMap && ptr->source.IsValid()) {
        // Try to get the association
        if (SpvSourceAssociation sourceAssociation = sourceMap->GetSourceAssociation(ptr->source.codeOffset)) {
            mapping.fileUID = sourceAssociation.fileUID;
            mapping.line = sourceAssociation.line;
            mapping.column = sourceAssociation.column;
        }
    }

    // Serial
    std::lock_guard guard(mutex);

    // Get entry
    ShaderEntry& shaderEntry = shaderEntries[mapping.shaderGUID];

    // Find mapping
    auto ssmIt = shaderEntry.mappings.find(mapping);
    if (ssmIt == shaderEntry.mappings.end()) {
        // Free indices?
        if (!freeIndices.empty()) {
            mapping.sguid = freeIndices.back();
            freeIndices.pop_back();
        }

        // May allocate?
        else if (counter < (1u << kShaderSGUIDBitCount)) {
            mapping.sguid = counter++;
        }

        // Out of indices
        else {
            return InvalidShaderSGUID;
        }

        // Add to pending
        pendingSubmissions.push_back(mapping.sguid);

        // Insert mappings
        shaderEntry.mappings[mapping] = mapping;
        sguidLookup.at(mapping.sguid) = mapping;
        return mapping.sguid;
    }

    // Return the SGUID
    return ssmIt->second.sguid;
}

ShaderSourceMapping ShaderSGUIDHost::GetMapping(ShaderSGUID sguid) {
    std::lock_guard guard(mutex);
    return sguidLookup.at(sguid);
}

const SpvSourceMap *ShaderSGUIDHost::GetSourceMap(uint64_t shaderGUID) {
    ShaderModuleState* shader = table->states_shaderModule.GetFromUID(shaderGUID);
    if (!shader) {
        return nullptr;
    }

    if (!shader->spirvModule) {
        return nullptr;
    }

    const SpvSourceMap* sourceMap = shader->spirvModule->GetSourceMap();
    ASSERT(sourceMap, "Source map must have been initialized");

    return sourceMap;
}

std::string_view ShaderSGUIDHost::GetSource(ShaderSGUID sguid) {
    if (sguid == InvalidShaderSGUID) {
        return {};
    }

    std::lock_guard guard(mutex);
    return GetSource(sguidLookup.at(sguid));
}

std::string_view ShaderSGUIDHost::GetSource(const ShaderSourceMapping &mapping) {
    // May not be mapped (IL only)
    if (mapping.fileUID == kInvalidShaderSourceFileUID) {
        return {};
    }

    // Get source map
    const SpvSourceMap* map = GetSourceMap(mapping.shaderGUID);

    // Get line
    std::string_view view = map->GetLine(mapping.fileUID, mapping.line);

    // Default to no column offsets
#if 1
    // Cut whitespaces if possible
    if (auto it = view.find_first_not_of(" \t\r\n"); it != std::string::npos) {
        return view.substr(it);
    } else {
        return view;
    }
#else // 1
    // To column
    uint32_t column = std::min<uint32_t>(mapping.column, std::max<uint32_t>(static_cast<uint32_t>(view.length()), 1) - 1);

    // Offset by column
    return view.substr(column, view.length() - column);
#endif // 1
}
