#include <Backends/Vulkan/Controllers/MetadataController.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/ShaderModuleState.h>
#include <Backends/Vulkan/Compiler/SpvModule.h>
#include <Backends/Vulkan/Compiler/SpvSourceMap.h>
#include <Backends/Vulkan/Symbolizer/ShaderSGUIDHost.h>

// Bridge
#include <Bridge/IBridge.h>

// Message
#include <Message/IMessageStorage.h>

// Backend
#include <Backend/IL/PrettyPrint.h>

// Schemas
#include <Schemas/SGUID.h>
#include <Schemas/ShaderMetadata.h>

// Std
#include <sstream>

MetadataController::MetadataController(DeviceDispatchTable *table) : table(table) {

}

bool MetadataController::Install() {
    // Install bridge
    bridge = registry->Get<IBridge>().GetUnsafe();
    if (!bridge) {
        return false;
    }

    // Install this listener
    bridge->Register(this);

    // OK
    return true;
}

void MetadataController::Uninstall() {
    // Uninstall this listener
    bridge->Deregister(this);
}

void MetadataController::Handle(const MessageStream *streams, uint32_t count) {
    std::lock_guard guard(mutex);

    for (uint32_t i = 0; i < count; i++) {
        ConstMessageStreamView view(streams[i]);

        // Visit all ordered messages
        for (ConstMessageStreamView<>::ConstIterator it = view.GetIterator(); it; ++it) {
            switch (it.GetID()) {
                case GetShaderCodeMessage::kID: {
                    OnMessage(*it.Get<GetShaderCodeMessage>());
                    break;
                }
                case GetShaderGUIDSMessage::kID: {
                    OnMessage(*it.Get<GetShaderGUIDSMessage>());
                    break;
                }
                case GetShaderSourceMappingMessage::kID: {
                    OnMessage(*it.Get<GetShaderSourceMappingMessage>());
                    break;
                }
            }
        }
    }
}

void MetadataController::OnMessage(const GetShaderCodeMessage& message) {
    MessageStreamView view(stream);

    // Attempt to find shader with given UID
    ShaderModuleState* shader = table->states_shaderModule.GetFromUID(message.shaderUID);

    // Failed?
    if (!shader || !shader->spirvModule) {
        auto&& response = view.Add<ShaderCodeMessage>();
        response->shaderUID = message.shaderUID;
        response->found = false;
        return;
    }

    // Get source map
    const SpvSourceMap* sourceMap = shader->spirvModule->GetSourceMap();

    // No sources available?
    if (!sourceMap || !sourceMap->GetFileCount()) {
        // Add response
        auto&& response = view.Add<ShaderCodeMessage>();
        response->shaderUID = message.shaderUID;
        response->found = true;
        response->native = true;
        response->fileCount = 1;

        // Pretty print to stream
        std::stringstream ilStream;
        IL::PrettyPrint(*shader->spirvModule->GetProgram(), ilStream);

        // Add native file
        auto&& file = view.Add<ShaderCodeFileMessage>(ShaderCodeFileMessage::AllocationInfo { .codeLength = static_cast<size_t>(ilStream.tellp()) });
        file->shaderUID = message.shaderUID;
        file->fileUID = 0;
        file->code.Set(ilStream.str());
        return;
    }

    // Number of files
    const uint32_t fileCount = sourceMap->GetFileCount();

    // Add response
    auto&& response = view.Add<ShaderCodeMessage>();
    response->shaderUID = message.shaderUID;
    response->found = true;
    response->native = false;
    response->fileCount = fileCount;

    // Add file responses
    for (uint32_t i = 0; i < fileCount; i++) {
        auto&& file = view.Add<ShaderCodeFileMessage>(ShaderCodeFileMessage::AllocationInfo { 
            .filenameLength = sourceMap->GetFilename().length(),
            .codeLength = sourceMap->GetCombinedSourceLength(i)
        });
        file->shaderUID = message.shaderUID;
        file->fileUID = i;

        // Fill filename
        file->filename.Set(sourceMap->GetFilename());

        // Fill discontinuous fragments into buffer
        sourceMap->FillCombinedSource(i, file->code.data.Get());
    }
}

void MetadataController::OnMessage(const struct GetShaderGUIDSMessage& message) {
    MessageStreamView view(stream);

    // Get linear (locked) view
    auto&& linear = table->states_shaderModule.GetLinear();

    // Throttle count
    const uint32_t count = std::min(message.limit, static_cast<uint32_t>(linear.object.size()));

    // Add response
    auto&& response = view.Add<ShaderGUIDSMessage>(ShaderGUIDSMessage::AllocationInfo { .shaderUIDCount = count });

    // Fill uids
    for (size_t i = 0; i < count; i++) {
        response->shaderUID[i] = linear[i]->uid;
    }
}

void MetadataController::OnMessage(const struct GetShaderSourceMappingMessage& message) {
    MessageStreamView<ShaderSourceMappingMessage> view(segmentMappingStream);

    // Get mapping
    ShaderSourceMapping mapping = table->sguidHost->GetMapping(message.sguid);

    // Get contents
    std::string_view sourceContents = table->sguidHost->GetSource(message.sguid);

    // Add response
    ShaderSourceMappingMessage* response = view.Add(ShaderSourceMappingMessage::AllocationInfo{ 
        .contentsLength = sourceContents.length()
    });
    response->sguid = message.sguid;
    response->shaderGUID = mapping.shaderGUID;
    response->fileUID = mapping.fileUID;
    response->line = mapping.line;
    response->column = mapping.column;

    // Fill contents
    response->contents.Set(sourceContents);
}

void MetadataController::Commit() {
    std::lock_guard guard(mutex);

    // Export general to bridge
    bridge->GetOutput()->AddStreamAndSwap(stream);
    bridge->GetOutput()->AddStreamAndSwap(segmentMappingStream);
}
