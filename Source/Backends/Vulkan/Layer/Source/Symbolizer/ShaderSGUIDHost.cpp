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

    std::lock_guard guard(mutex);

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
    const SpvSourceMap* map = GetSourceMap(mapping.shaderGUID);

    // Get line
    std::string_view view = map->GetLine(mapping.fileUID, mapping.line);

    // To column
    uint32_t column = std::min<uint32_t>(mapping.column, std::max<uint32_t>(static_cast<uint32_t>(view.length()), 1) - 1);

    // Offset by column
    return view.substr(column, view.length() - column);
}
