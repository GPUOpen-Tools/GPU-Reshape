#include <Backends/Vulkan/Symbolizer/ShaderSGUIDHost.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/ShaderModuleState.h>
#include <Backends/Vulkan/Compiler/SpvModule.h>
#include <Backends/Vulkan/Compiler/SpvSourceMap.h>

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

    // Write all pending
    for (ShaderSGUID sguid : pendingSubmissions) {
        std::string_view sourceContents = GetSource(sguid);

        // Allocate message
        ShaderSourceMappingMessage* message = view.Add(ShaderSourceMappingMessage::AllocationInfo {
            .contentsLength = sourceContents.length()
        });

        // Set SGUID
        message->sguid = sguid;

        // Fill mapping
        ShaderSourceMapping mapping = GetMapping(sguid);
        message->shaderGUID = mapping.shaderGUID;
        message->fileUID = mapping.fileUID;
        message->line = mapping.line;
        message->column = mapping.column;

        // Fill contents
        message->contents.Set(sourceContents);
    }

    // Export to bridge
    bridge->GetOutput()->AddStream(stream);

    // Reset
    pendingSubmissions.clear();
}

ShaderSGUID ShaderSGUIDHost::Bind(const IL::Program &program, const IL::ConstOpaqueInstructionRef& instruction) {
    // Get instruction pointer
    const IL::Instruction* ptr = IL::ConstInstructionRef<>(instruction).Get();

    // Find the source map
    const SpvSourceMap* sourceMap = GetSourceMap(program.GetShaderGUID());
    if (!sourceMap) {
        return InvalidShaderSGUID;
    }

    // Must have source
    if (!ptr->source.IsValid()) {
        return InvalidShaderSGUID;
    }

    // Try to get the association
    SpvSourceAssociation sourceAssociation = sourceMap->GetSourceAssociation(ptr->source.codeOffset);
    if (!sourceAssociation) {
        return InvalidShaderSGUID;
    }

    // Create mapping
    ShaderSourceMapping mapping{};
    mapping.fileUID = sourceAssociation.fileUID;
    mapping.line = sourceAssociation.line;
    mapping.column = sourceAssociation.column;
    mapping.shaderGUID = program.GetShaderGUID();

    // Get entry
    ShaderEntry& shaderEntry = shaderEntries[mapping.shaderGUID];

    // Find mapping
    auto ssmIt = shaderEntry.mappings.find(mapping.GetInlineSortKey());
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
        shaderEntry.mappings[mapping.GetInlineSortKey()] = mapping;
        sguidLookup.at(mapping.sguid) = mapping;
        return mapping.sguid;
    }

    // Return the SGUID
    return ssmIt->second.sguid;
}

ShaderSourceMapping ShaderSGUIDHost::GetMapping(ShaderSGUID sguid) {
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
    return GetSource(sguidLookup.at(sguid));
}

std::string_view ShaderSGUIDHost::GetSource(const ShaderSourceMapping &mapping) {
    const SpvSourceMap* map = GetSourceMap(mapping.shaderGUID);

    std::string_view view = map->GetLine(mapping.fileUID, mapping.line);
    ASSERT(mapping.column < view.length(), "Column exceeds line length");

    // Offset by column
    return view.substr(mapping.column, view.length() - mapping.column);
}
