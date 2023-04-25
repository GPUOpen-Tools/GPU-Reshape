#include <Backends/Vulkan/Controllers/MetadataController.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/ShaderModuleState.h>
#include <Backends/Vulkan/States/PipelineState.h>
#include <Backends/Vulkan/Compiler/SpvModule.h>
#include <Backends/Vulkan/Compiler/SpvSourceMap.h>
#include <Backends/Vulkan/Compiler/ShaderCompiler.h>
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
#include <Schemas/PipelineMetadata.h>
#include <Schemas/Object.h>

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

    // Get components
    shaderCompiler = registry->Get<ShaderCompiler>();

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
                case GetPipelineNameMessage::kID: {
                    OnMessage(*it.Get<GetPipelineNameMessage>());
                    break;
                }
                case GetShaderCodeMessage::kID: {
                    OnMessage(*it.Get<GetShaderCodeMessage>());
                    break;
                }
                case GetShaderILMessage::kID: {
                    OnMessage(*it.Get<GetShaderILMessage>());
                    break;
                }
                case GetShaderBlockGraphMessage::kID: {
                    OnMessage(*it.Get<GetShaderBlockGraphMessage>());
                    break;
                }
                case GetShaderUIDRangeMessage::kID: {
                    OnMessage(*it.Get<GetShaderUIDRangeMessage>());
                    break;
                }
                case GetPipelineUIDRangeMessage::kID: {
                    OnMessage(*it.Get<GetPipelineUIDRangeMessage>());
                    break;
                }
                case GetObjectStatesMessage::kID: {
                    OnMessage(*it.Get<GetObjectStatesMessage>());
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

void MetadataController::OnMessage(const GetPipelineNameMessage& message) {
    MessageStreamView view(stream);

    // Attempt to find shader with given UID
    PipelineState* pipeline = table->states_pipeline.GetFromUID(message.pipelineUID);

    // Determine name
    const char* name = pipeline->debugName ? pipeline->debugName : "Unknown";

    // Push response
    auto&& file = view.Add<PipelineNameMessage>(PipelineNameMessage::AllocationInfo { .nameLength = static_cast<size_t>(std::strlen(name)) });
    file->pipelineUID = message.pipelineUID;
    file->name.Set(name);
}

void MetadataController::OnMessage(const GetShaderCodeMessage& message) {
    MessageStreamView view(stream);

    // Attempt to find shader with given UID
    ShaderModuleState* shader = table->states_shaderModule.GetFromUID(message.shaderUID);

    // Create module if not present
    if (shaderCompiler && !shader->spirvModule) {
        shaderCompiler->InitializeModule(shader);
    }

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
        response->fileCount = 0;
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
            .filenameLength = sourceMap->GetSourceFilename(i).length(),
            .codeLength = message.poolCode ? sourceMap->GetCombinedSourceLength(i) : 0
        });
        file->shaderUID = message.shaderUID;
        file->fileUID = i;

        // Fill filename
        file->filename.Set(sourceMap->GetSourceFilename(i));

        // Fill discontinuous fragments into buffer
        if (message.poolCode) {
            sourceMap->FillCombinedSource(i, file->code.data.Get());
        }
    }
}

void MetadataController::OnMessage(const GetShaderILMessage& message) {
    MessageStreamView view(stream);

    // Attempt to find shader with given UID
    ShaderModuleState* shader = table->states_shaderModule.GetFromUID(message.shaderUID);

    // Create module if not present
    if (shaderCompiler && !shader->spirvModule) {
        shaderCompiler->InitializeModule(shader);
    }

    // Failed?
    if (!shader || !shader->spirvModule) {
        auto&& response = view.Add<ShaderILMessage>();
        response->shaderUID = message.shaderUID;
        response->found = false;
        return;
    }

    // Pretty print to stream
    std::stringstream ilStream;
    IL::PrettyPrint(*shader->spirvModule->GetProgram(), ilStream);

    // Add native file
    auto&& file = view.Add<ShaderILMessage>(ShaderILMessage::AllocationInfo { .codeLength = static_cast<size_t>(ilStream.tellp()) });
    file->shaderUID = message.shaderUID;
    file->found = true;
    file->code.Set(ilStream.str());
}

void MetadataController::OnMessage(const GetShaderBlockGraphMessage& message) {
    MessageStreamView view(stream);

    // Attempt to find shader with given UID
    ShaderModuleState* shader = table->states_shaderModule.GetFromUID(message.shaderUID);

    // Create module if not present
    if (shaderCompiler && !shader->spirvModule) {
        shaderCompiler->InitializeModule(shader);
    }

    // Failed?
    if (!shader || !shader->spirvModule) {
        auto&& response = view.Add<ShaderBlockGraphMessage>();
        response->shaderUID = message.shaderUID;
        response->found = false;
        return;
    }

    // Pretty print to stream
    std::stringstream blockStream;
    
    // Open function block
    blockStream << "{";
    blockStream << "\t\"functions\": \n";
    blockStream << "\t[";

    // Get functions
    IL::FunctionList& functions = shader->spirvModule->GetProgram()->GetFunctionList();

    // Print graph
    for (auto it = functions.begin(); it != functions.end(); it++) {
        if (it != functions.begin()) {
            blockStream << ",\n\n";
        } else {
            blockStream << "\n";
        }
        
        IL::PrettyPrintBlockJsonGraph(**it, blockStream);
    }

    // Close function block
    blockStream << "\t]\n";
    blockStream << "}";

    // Add graph file
    auto&& file = view.Add<ShaderBlockGraphMessage>(ShaderBlockGraphMessage::AllocationInfo { .nodesLength = static_cast<size_t>(blockStream.tellp()) });
    file->shaderUID = message.shaderUID;
    file->found = true;
    file->nodes.Set(blockStream.str());
}

void MetadataController::OnMessage(const struct GetObjectStatesMessage& message) {
    MessageStreamView view(stream);

    // Add response (linear locks)
    auto&& response = view.Add<ObjectStatesMessage>();
    response->shaderCount = static_cast<uint32_t>(table->states_shaderModule.GetLinear().object.size());
    response->pipelineCount = static_cast<uint32_t>(table->states_shaderModule.GetLinear().object.size());
}

void MetadataController::OnMessage(const struct GetShaderUIDRangeMessage& message) {
    MessageStreamView view(stream);

    // Get linear (locked) view
    auto&& linear = table->states_shaderModule.GetLinear();

    // Out of range?
    if (message.start >= linear.object.size()) {
        view.Add<ShaderUIDRangeMessage>(ShaderUIDRangeMessage::AllocationInfo { .shaderUIDCount = 0 });
        return;
    }

    // Throttle count
    const uint32_t count = std::min(message.limit, static_cast<uint32_t>(linear.object.size() - message.start));

    // Add response
    auto&& response = view.Add<ShaderUIDRangeMessage>(ShaderUIDRangeMessage::AllocationInfo { .shaderUIDCount = count });
    response->start = message.start;

    // Fill uids
    for (size_t i = 0; i < count; i++) {
        response->shaderUID[i] = linear[message.start + i]->uid;
    }
}

void MetadataController::OnMessage(const struct GetPipelineUIDRangeMessage& message) {
    MessageStreamView view(stream);

    // Get linear (locked) view
    auto&& linear = table->states_pipeline.GetLinear();

    // Out of range?
    if (message.start >= linear.object.size()) {
        view.Add<PipelineUIDRangeMessage>(PipelineUIDRangeMessage::AllocationInfo { .pipelineUIDCount = 0 });
        return;
    }

    // Throttle count
    const uint32_t count = std::min(message.limit, static_cast<uint32_t>(linear.object.size() - message.start));

    // Add response
    auto&& response = view.Add<PipelineUIDRangeMessage>(PipelineUIDRangeMessage::AllocationInfo { .pipelineUIDCount = count });
    response->start = message.start;

    // Fill uids
    for (size_t i = 0; i < count; i++) {
        response->pipelineUID[i] = linear[message.start + i]->uid;
    }
}

void MetadataController::OnMessage(const struct GetShaderSourceMappingMessage& message) {
    MessageStreamView<ShaderSourceMappingMessage> view(segmentMappingStream);

    // Get mapping
    ShaderSourceMapping mapping = table->sguidHost->GetMapping(static_cast<uint32_t>(message.sguid));

    // Get contents
    std::string_view sourceContents = table->sguidHost->GetSource(static_cast<uint32_t>(message.sguid));

    // Add response
    ShaderSourceMappingMessage* response = view.Add(ShaderSourceMappingMessage::AllocationInfo{ 
        .contentsLength = sourceContents.length()
    });
    response->sguid = static_cast<uint32_t>(message.sguid);
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
