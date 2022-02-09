#include <Backends/Vulkan/Controllers/InstrumentationController.h>
#include <Backends/Vulkan/Compiler/ShaderCompiler.h>
#include <Backends/Vulkan/Compiler/PipelineCompiler.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/ShaderModuleState.h>
#include <Backends/Vulkan/States/PipelineLayoutState.h>
#include <Backends/Vulkan/States/PipelineState.h>
#include <Backends/Vulkan/CommandBuffer.h>
#include <Backends/Vulkan/Symbolizer/ShaderSGUIDHost.h>

// Bridge
#include <Bridge/IBridge.h>

// Common
#include <Common/Registry.h>
#include <Common/Dispatcher/TaskGroup.h>

// Schemas
#include <Schemas/Pipeline.h>
#include <Schemas/Config.h>
#include <Schemas/ShaderMetadata.h>

InstrumentationController::InstrumentationController(DeviceDispatchTable *table) : table(table) {

}

bool InstrumentationController::Install() {
    shaderCompiler = registry->Get<ShaderCompiler>();
    pipelineCompiler = registry->Get<PipelineCompiler>();
    dispatcher = registry->Get<Dispatcher>();

    auto bridge = registry->Get<IBridge>();
    bridge->Register(this);

    return true;
}

void InstrumentationController::Uninstall() {
    auto bridge = registry->Get<IBridge>();
    bridge->Deregister(this);
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
    // TODO: Tie lifetime of this task group to the controller
    TaskGroup group(dispatcher.GetUnsafe());
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

    // Submit compiler jobs
    for (ShaderModuleState* state : batch->dirtyShaderModules) {
        uint64_t shaderFeatureBitSet = globalInstrumentationInfo.featureBitSet | state->instrumentationInfo.featureBitSet;

        // Perform feedback from the dependent objects
        for (PipelineState* dependentObject : table->dependencies_shaderModulesPipelines.Get(state)) {
            // Get the super feature set
            uint64_t featureBitSet = shaderFeatureBitSet | dependentObject->instrumentationInfo.featureBitSet;

            // Number of slots used by the pipeline
            uint32_t pipelineLayoutUserSlots = dependentObject->layout->boundUserDescriptorStates;

            // Create the instrumentation key
            ShaderModuleInstrumentationKey instrumentationKey{};
            instrumentationKey.featureBitSet = featureBitSet;
            instrumentationKey.pipelineLayoutUserSlots = pipelineLayoutUserSlots;

            // Attempt to reserve
            if (!state->Reserve(instrumentationKey)) {
                continue;
            }

            // Inject the feedback state
            shaderCompiler->Add(table, state, instrumentationKey, bucket);
        }
    }
}

void InstrumentationController::CommitPipelines(DispatcherBucket* bucket, void *data) {
    auto* batch = static_cast<Batch*>(data);

    // Allocate batch
    auto jobs = new (registry->GetAllocators()) PipelineJob[batch->dirtyPipelines.size()];

    // Submit compiler jobs
    for (size_t i = 0; i < batch->dirtyPipelines.size(); i++) {
        PipelineState* state = batch->dirtyPipelines[i];

        // Setup the job
        PipelineJob& job = jobs[i];
        job.state = state;
        job.featureBitSet = globalInstrumentationInfo.featureBitSet | state->instrumentationInfo.featureBitSet;

        // Allocate feature bit sets
        job.shaderModuleInstrumentationKeys = new (registry->GetAllocators()) ShaderModuleInstrumentationKey[state->shaderModules.size()];

        // Set the module feature bit sets
        for (uint32_t shaderIndex = 0; shaderIndex < state->shaderModules.size(); shaderIndex++) {
            uint64_t featureBitSet = 0;

            // Create super feature bit set (global -> shader -> pipeline)
            // ? Pipeline specific bit set fed back during shader compilation
            featureBitSet |= globalInstrumentationInfo.featureBitSet;
            featureBitSet |= state->shaderModules[i]->instrumentationInfo.featureBitSet;
            featureBitSet |= state->instrumentationInfo.featureBitSet;

            // Number of slots used by the pipeline
            uint32_t pipelineLayoutUserSlots = state->layout->boundUserDescriptorStates;

            // Create the instrumentation key
            ShaderModuleInstrumentationKey instrumentationKey{};
            instrumentationKey.featureBitSet = featureBitSet;
            instrumentationKey.pipelineLayoutUserSlots = pipelineLayoutUserSlots;

            // Assign key
            job.shaderModuleInstrumentationKeys[shaderIndex] = instrumentationKey;
        }
    }

    // Submit all jobs
    pipelineCompiler->AddBatch(table, jobs, static_cast<uint32_t>(batch->dirtyPipelines.size()), bucket);

    // Free up
    destroy(jobs, registry->GetAllocators());
}

void InstrumentationController::CommitTable(DispatcherBucket* bucket, void *data) {
    auto* batch = static_cast<Batch*>(data);

    // Commit all sguid changes
    auto bridge = registry->Get<IBridge>();
    table->sguidHost->Commit(bridge.GetUnsafe());

    // Set the enabled feature bit set
    SetDeviceCommandFeatureSetAndCommit(table, batch->featureBitSet);

    // Mark as done
    compilationEvent.IncrementCounter();

    // Release batch
    destroy(batch, allocators);
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
