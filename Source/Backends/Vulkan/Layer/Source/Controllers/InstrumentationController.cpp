#include <Backends/Vulkan/Controllers/InstrumentationController.h>
#include <Backends/Vulkan/Shader/ShaderCompiler.h>
#include <Backends/Vulkan/DeviceDispatchTable.h>
#include <Backends/Vulkan/ShaderModuleState.h>
#include <Backends/Vulkan/PipelineState.h>
#include <Backends/Vulkan/CommandBuffer.h>

// Common
#include <Common/Registry.h>
#include <Common/TaskGroup.h>

// Schemas
#include <Schemas/Pipeline.h>
#include <Schemas/Config.h>
#include <Schemas/ShaderMetadata.h>

/**
 * TODO: This really isn't ready yet
 *
 *  Submission should include the enabled bit set, it has to. Otherwise we risk race conditions with the enabled feature sets.
 */

InstrumentationController::InstrumentationController(Registry* registry, DeviceDispatchTable *table) : registry(registry), table(table) {
    shaderCompiler = registry->Get<ShaderCompiler>();
    dispatcher = registry->Get<Dispatcher>();
}

void InstrumentationController::Handle(const MessageStream *streams, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        ConstMessageStreamView view(streams[i]);

        // Visit all ordered messages
        for (ConstMessageStreamView<>::ConstIterator it = view.GetIterator(); it; ++it) {
            OnMessage(it);
        }
    }

    if (!immediateBatch.dirtyObjects.empty()) {
        Commit();
    }
}

void InstrumentationController::OnMessage(const ConstMessageStreamView<>::ConstIterator &it) {
    switch (it.GetID()) {
        // Config
        case SetInstrumentationConfigMessage::kID: {
            auto *message = it.Get<SetInstrumentationConfigMessage>();
            synchronousRecording = message->synchronousRecording;
            break;
        }

        // Global instrumentation
        case SetGlobalInstrumentationMessage::kID: {
            auto *message = it.Get<SetGlobalInstrumentationMessage>();

            // Apply instrumentation
            SetInstrumentationInfo(globalInstrumentationInfo, message->featureBitSet, message->specialization);

            // Add all shader modules
            for (ShaderModuleState *state: table->states_shaderModule.GetLinear()) {
                if (immediateBatch.dirtyObjects.count(state)) {
                    continue;
                }

                immediateBatch.dirtyObjects.insert(state);
                immediateBatch.dirtyShaderModules.push_back(state);
            }

            // Add all pipelines modules
            for (PipelineState *state: table->states_pipeline.GetLinear()) {
                if (immediateBatch.dirtyObjects.count(state)) {
                    continue;
                }

                immediateBatch.dirtyObjects.insert(state);
                immediateBatch.dirtyPipelines.push_back(state);
            }
            break;
        }

            // Shader instrumentation
        case SetShaderInstrumentationMessage::kID: {
            auto *message = it.Get<SetShaderInstrumentationMessage>();

            // Attempt to get the state
            ShaderModuleState *state = table->states_shaderModule.GetFromUID(message->shaderUID);
            if (!state) {
                // TODO: Error logging
                return;
            }

            // Apply instrumentation
            SetInstrumentationInfo(state->instrumentationInfo, message->featureBitSet, message->specialization);

            // Add the state itself
            if (!immediateBatch.dirtyObjects.count(state)) {
                immediateBatch.dirtyObjects.insert(state);
                immediateBatch.dirtyShaderModules.push_back(state);
            }

            // Add dependent pipelines
            for (PipelineState *dependentState: table->dependencies_shaderModulesPipelines.Get(state)) {
                if (immediateBatch.dirtyObjects.count(dependentState)) {
                    continue;
                }

                immediateBatch.dirtyObjects.insert(dependentState);
                immediateBatch.dirtyPipelines.push_back(dependentState);
            }
            break;
        }

            // Pipeline instrumentation
        case SetPipelineInstrumentationMessage::kID: {
            auto *message = it.Get<SetPipelineInstrumentationMessage>();

            // Attempt to get the state
            PipelineState *state = table->states_pipeline.GetFromUID(message->pipelineUID);
            if (!state) {
                // TODO: Error logging
                return;
            }

            // Apply instrumentation
            SetInstrumentationInfo(state->instrumentationInfo, message->featureBitSet, message->specialization);

            // Add the state itself
            if (!immediateBatch.dirtyObjects.count(state)) {
                immediateBatch.dirtyObjects.insert(state);
                immediateBatch.dirtyPipelines.push_back(state);
            }

            // Add source modules
            for (ShaderModuleState *shaderModuleState: state->shaderModules) {
                immediateBatch.dirtyObjects.insert(shaderModuleState);
                immediateBatch.dirtyShaderModules.push_back(shaderModuleState);
            }
            break;
        }
    }
}

void InstrumentationController::SetInstrumentationInfo(InstrumentationInfo &info, uint64_t bitSet, const MessageSubStream &stream) {
    // Set the enabled bit-set
    info.featureBitSet = bitSet;

    // Transfer sub stream
    stream.Transfer(info.specialization);
}

void InstrumentationController::Commit() {
    compilationEvent.IncrementHead();

    // Copy batch
    auto* batch = new (registry->GetAllocators()) Batch(immediateBatch);

    // Summarize the needed feature set
    batch->featureBitSet = SummarizeFeatureBitSet();

    // Task group
    TaskGroup group(dispatcher);
    group.Chain(BindDelegate(this, InstrumentationController::CommitShaders), batch);
    group.Chain(BindDelegate(this, InstrumentationController::CommitPipelines), batch);
    group.Chain(BindDelegate(this, InstrumentationController::CommitTable), batch);

    // Start the group
    group.Commit();

    // Clean current batch
    immediateBatch.dirtyObjects.clear();
    immediateBatch.dirtyShaderModules.clear();
    immediateBatch.dirtyPipelines.clear();
}

void InstrumentationController::CommitShaders(DispatcherBucket* bucket, void *data) {
    auto* batch = static_cast<Batch*>(data);

    // Set completion
    bucket->AddCounter(batch->dirtyShaderModules.size());

    // Submit compiler jobs
    for (ShaderModuleState* state : batch->dirtyShaderModules) {
        uint64_t featureSet = globalInstrumentationInfo.featureBitSet | state->instrumentationInfo.featureBitSet;

        // TODO: Not accurate, pipeline needs to feed back!
        shaderCompiler->Add(state, featureSet, bucket);
    }
}

void InstrumentationController::CommitPipelines(DispatcherBucket* bucket, void *data) {
    // Under construction...
}

void InstrumentationController::CommitTable(DispatcherBucket* bucket, void *data) {
    auto* batch = static_cast<Batch*>(data);

    // Set the enabled feature bit set
    SetDeviceCommandFeatureSet(table, batch->featureBitSet);

    // Mark as done
    compilationEvent.IncrementCounter();
}

uint64_t InstrumentationController::SummarizeFeatureBitSet() {
    uint64_t featureBitSet = globalInstrumentationInfo.featureBitSet;

    // Note: Easier than keeping track of all the states, and far less error-prone

    // Summarize all shaders
    for (ShaderModuleState *state: table->states_shaderModule.GetLinear()) {
        featureBitSet |= state->instrumentationInfo.featureBitSet;
    }

    // Summarize all pipelines
    for (PipelineState *state: table->states_pipeline.GetLinear()) {
        featureBitSet |= state->instrumentationInfo.featureBitSet;
    }

    return featureBitSet;
}

void InstrumentationController::BeginCommandBuffer() {
    // If syncronous, wait for the head compilation counter
    if (synchronousRecording) {
        compilationEvent.Wait(compilationEvent.GetHead());
    }
}
