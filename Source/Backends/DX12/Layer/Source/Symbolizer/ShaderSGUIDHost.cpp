// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

#include <Backends/DX12/Symbolizer/ShaderSGUIDHost.h>
#include <Backends/DX12/Compiler/IDXDebugModule.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/States/ShaderState.h>
#include <Backends/DX12/Compiler/IDXModule.h>
#include <Backends/DX12/Compiler/IDXDebugModule.h>
#include <Backends/DX12/Compiler/DXCodeOffsetTraceback.h>

// Backend
#include <Backend/IL/Program.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStream.h>

// Bridge
#include <Bridge/IBridge.h>

// Schemas
#include <Schemas/SGUID.h>

ShaderSGUIDHost::ShaderSGUIDHost(DeviceState *device) :
    device(device),
    freeIndices(device->allocators.Tag(kAllocSGUID)),
    sguidLookup(device->allocators.Tag(kAllocSGUID)),
    pendingSubmissions(device->allocators.Tag(kAllocSGUID)) {

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

    // Must have source
    if (!ptr->source.IsValid()) {
        return InvalidShaderSGUID;
    }

    // Get shader state
    ShaderState* shaderState = device->states_Shaders.GetFromUID(program.GetShaderGUID());
    if (!shaderState || !shaderState->module) {
        return InvalidShaderSGUID;
    }

    // Get traceback
    DXCodeOffsetTraceback traceback = shaderState->module->GetCodeOffsetTraceback(ptr->source.codeOffset);

    // Default mapping
    ShaderSourceMapping mapping{};
    mapping.shaderGUID = program.GetShaderGUID();

    // Mapping il association
    mapping.basicBlockId = traceback.basicBlockID;
    mapping.instructionIndex = traceback.instructionIndex;
    
    // Debug modules are optional
    if (IDXDebugModule* debugModule = shaderState->module->GetDebug()) {
        // Try to get the association
        DXSourceAssociation sourceAssociation = debugModule->GetSourceAssociation(ptr->source.codeOffset);
        if (!sourceAssociation) {
            return InvalidShaderSGUID;
        }

        // Mapping source association
        mapping.fileUID = sourceAssociation.fileUID;
        mapping.line = sourceAssociation.line;
        mapping.column = sourceAssociation.column;
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

std::string_view ShaderSGUIDHost::GetSource(ShaderSGUID sguid) {
    if (sguid == InvalidShaderSGUID) {
        return {};
    }

    std::lock_guard guard(mutex);
    return GetSource(sguidLookup.at(sguid));
}

std::string_view ShaderSGUIDHost::GetSource(const ShaderSourceMapping &mapping) {
    // Get shader state
    ShaderState* shaderState = device->states_Shaders.GetFromUID(mapping.shaderGUID);
    if (!shaderState || !shaderState->module) {
        return {};
    }

    // Debug modules are optional
    IDXDebugModule* debugModule = shaderState->module->GetDebug();
    if (!debugModule) {
        return {};
    }

    // Get view for line
    std::string_view view = debugModule->GetLine(mapping.fileUID, mapping.line);

    // Default to no column offsets
#if 1
    // Cut whitespaces if possible
    if (auto it = view.find_first_not_of(" \t\r\n"); it != std::string::npos) {
        return view.substr(it);
    } else {
        return view;
    }
#else // 1
    // Safe column count
    uint32_t column = std::min<uint32_t>(mapping.column, std::max(static_cast<uint32_t>(view.length()), 1u) - 1);

    // Offset by column
    return view.substr(column, view.length() - column);
#endif // 1
}
