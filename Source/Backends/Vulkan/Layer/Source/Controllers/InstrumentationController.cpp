#include <Backends/Vulkan/Controllers/InstrumentationController.h>
#include <Backends/Vulkan/Compiler/ShaderCompiler.h>
#include <Backends/Vulkan/Compiler/PipelineCompiler.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>
#include <Backends/Vulkan/States/ShaderModuleState.h>
#include <Backends/Vulkan/States/PipelineLayoutState.h>
#include <Backends/Vulkan/States/PipelineState.h>
#include <Backends/Vulkan/CommandBuffer.h>
#include <Backends/Vulkan/Symbolizer/ShaderSGUIDHost.h>

// Backend
#include <Backend/IFeature.h>

// Bridge
#include <Bridge/IBridge.h>

// Message
#include <Message/IMessageStorage.h>

// Bridge
#include <Common/Format.h>

// Common
#include <Common/Registry.h>
#include <Common/Dispatcher/TaskGroup.h>

// Schemas
#include <Schemas/Instrumentation.h>
#include <Schemas/Config.h>
#include <Schemas/ShaderMetadata.h>
#include <Schemas/Diagnostic.h>

// Std
#include <sstream>

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

uint32_t InstrumentationController::GetJobCount() {
    std::lock_guard guard(mutex);
    return compilationBucket ? compilationBucket->GetCounter() : 0;
}

void InstrumentationController::CreatePipeline(PipelineState *state) {
    std::lock_guard guard(mutex);

    // Propagate on state
    PropagateInstrumentationInfo(state);

    // Nothing of interest?
    if (!state->instrumentationInfo.featureBitSet) {
        return;
    }
    
    // Add the state itself
    if (!immediateBatch.dirtyObjects.count(state)) {
        immediateBatch.dirtyObjects.insert(state);
        immediateBatch.dirtyPipelines.push_back(state);

        // Own lifetime
        state->AddUser();
    }

    // Add source modules
    for (ShaderModuleState *shaderState: state->shaderModules) {
        if (immediateBatch.dirtyObjects.count(shaderState)) {
            continue;
        }

        // Own lifetime
        shaderState->AddUser();
                
        immediateBatch.dirtyObjects.insert(shaderState);
        immediateBatch.dirtyShaderModules.push_back(shaderState);
    }
}

void InstrumentationController::PropagateInstrumentationInfo(PipelineState *state) {
    state->instrumentationInfo.featureBitSet = 0x0;
    state->instrumentationInfo.specialization.Clear();

    // Append global
    state->instrumentationInfo.featureBitSet |= globalInstrumentationInfo.featureBitSet;
    state->instrumentationInfo.specialization.Append(globalInstrumentationInfo.specialization);

    // Append object specific
    if (auto it = pipelineUIDInstrumentationInfo.find(state->uid); it != pipelineUIDInstrumentationInfo.end()) {
        state->instrumentationInfo.featureBitSet |= it->second.featureBitSet;
        state->instrumentationInfo.specialization.Append(it->second.specialization);
    }

    // Append filters
    for (const FilterEntry& entry : filteredInstrumentationInfo) {
        if (!FilterPipeline(state, entry)) {
            continue;
        }

        // Passed!
        state->instrumentationInfo.featureBitSet |= entry.instrumentationInfo.featureBitSet;
        state->instrumentationInfo.specialization.Append(entry.instrumentationInfo.specialization);
    }
}

void InstrumentationController::PropagateInstrumentationInfo(ShaderModuleState *state) {
    state->instrumentationInfo.featureBitSet = 0x0;
    state->instrumentationInfo.specialization.Clear();

    // Append global
    state->instrumentationInfo.featureBitSet |= globalInstrumentationInfo.featureBitSet;
    state->instrumentationInfo.specialization.Append(globalInstrumentationInfo.specialization);

    // Append object specific
    if (auto it = shaderUIDInstrumentationInfo.find(state->uid); it != shaderUIDInstrumentationInfo.end()) {
        state->instrumentationInfo.featureBitSet |= it->second.featureBitSet;
        state->instrumentationInfo.specialization.Append(it->second.specialization);
    }
}

bool InstrumentationController::FilterPipeline(PipelineState *state, const FilterEntry &filter) {
    // Test type
    if (filter.type != PipelineType::Count && filter.type != state->type) {
        return false;
    }

    // Test name
    if (filter.name.data() && state->debugName && !std::strstr(state->debugName, filter.name.c_str())) {
        return false;
    }

    // Passed!
    return true;
}

void InstrumentationController::Handle(const MessageStream *streams, uint32_t count) {
    std::lock_guard guard(mutex);
    
    for (uint32_t i = 0; i < count; i++) {
        ConstMessageStreamView view(streams[i]);

        // Visit all ordered messages
        for (ConstMessageStreamView<>::ConstIterator it = view.GetIterator(); it; ++it) {
            OnMessage(it);
        }
    }

    if (!immediateBatch.dirtyObjects.empty()) {
        CommitInstrumentation();
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

        // Virtualization
        case SetVirtualFeatureRedirectMessage::kID: {
            auto *message = it.Get<SetVirtualFeatureRedirectMessage>();

            // Assume max
            if (virtualFeatureRedirects.size() != 64) {
                virtualFeatureRedirects.resize(64);
            }
            
            // Note: Not a free search, however, for the purposes of virtual redirects this is sufficient
            for (size_t i = 0; i < table->features.size(); i++) {
                if (table->features[i]->GetInfo().name == message->name.View()) {
                    virtualFeatureRedirects[message->index] = (1u << i);
                }
            }

            // Validate
            if (!virtualFeatureRedirects[message->index]) {
                table->parent->logBuffer.Add("Vulkan", Format("Virtual redirect failed for feature \"{}\"", message->name.View()));
            }
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

                // Own lifetime
                state->AddUser();

                immediateBatch.dirtyObjects.insert(state);
                immediateBatch.dirtyShaderModules.push_back(state);
            }

            // Add all pipelines modules
            for (PipelineState *state: table->states_pipeline.GetLinear()) {
                if (immediateBatch.dirtyObjects.count(state)) {
                    continue;
                }

                // Own lifetime
                state->AddUser();

                immediateBatch.dirtyObjects.insert(state);
                immediateBatch.dirtyPipelines.push_back(state);
            }
            break;
        }

            // Shader instrumentation
        case SetShaderInstrumentationMessage::kID: {
            auto *message = it.Get<SetShaderInstrumentationMessage>();

            // Apply instrumentation
            SetInstrumentationInfo(shaderUIDInstrumentationInfo[message->shaderUID], message->featureBitSet, message->specialization);

            // Attempt to get the state
            ShaderModuleState *state = table->states_shaderModule.GetFromUID(message->shaderUID);
            if (!state) {
                // TODO: Error logging
                return;
            }

            // Add the state itself
            if (!immediateBatch.dirtyObjects.count(state)) {
                immediateBatch.dirtyObjects.insert(state);
                immediateBatch.dirtyShaderModules.push_back(state);

                // Own lifetime
                state->AddUser();
            }

            // Add dependent pipelines
            for (PipelineState *dependentState: table->dependencies_shaderModulesPipelines.Get(state)) {
                if (immediateBatch.dirtyObjects.count(dependentState)) {
                    continue;
                }

                // Own lifetime
                dependentState->AddUser();

                immediateBatch.dirtyObjects.insert(dependentState);
                immediateBatch.dirtyPipelines.push_back(dependentState);
            }
            break;
        }

            // Pipeline instrumentation
        case SetPipelineInstrumentationMessage::kID: {
            auto *message = it.Get<SetPipelineInstrumentationMessage>();

            // Apply instrumentation
            SetInstrumentationInfo(pipelineUIDInstrumentationInfo[message->pipelineUID], message->featureBitSet, message->specialization);

            // Attempt to get the state
            PipelineState *state = table->states_pipeline.GetFromUID(message->pipelineUID);
            if (!state) {
                // TODO: Error logging
                return;
            }

            // Add the state itself
            if (!immediateBatch.dirtyObjects.count(state)) {
                immediateBatch.dirtyObjects.insert(state);
                immediateBatch.dirtyPipelines.push_back(state);

                // Own lifetime
                state->AddUser();
            }

            // Add source modules
            for (ShaderModuleState *shaderModuleState: state->shaderModules) {
                if (immediateBatch.dirtyObjects.count(shaderModuleState)) {
                    continue;
                }

                // Own lifetime
                shaderModuleState->AddUser();
                
                immediateBatch.dirtyObjects.insert(shaderModuleState);
                immediateBatch.dirtyShaderModules.push_back(shaderModuleState);
            }
            break;
        }

        case RemoveFilteredPipelineInstrumentationMessage::kID: {
            auto *message = it.Get<SetOrAddFilteredPipelineInstrumentationMessage>();

            // Remove all with matching guid
            filteredInstrumentationInfo.erase(std::ranges::remove_if(filteredInstrumentationInfo, [&](const FilterEntry& entry) {
                return entry.guid == message->guid.View();
            }).begin(), filteredInstrumentationInfo.end());

            // Add all shader modules
            for (ShaderModuleState *state: table->states_shaderModule.GetLinear()) {
                if (immediateBatch.dirtyObjects.count(state)) {
                    continue;
                }

                // Own lifetime
                state->AddUser();

                immediateBatch.dirtyObjects.insert(state);
                immediateBatch.dirtyShaderModules.push_back(state);
            }

            // Add all pipelines modules
            for (PipelineState *state: table->states_pipeline.GetLinear()) {
                if (immediateBatch.dirtyObjects.count(state)) {
                    continue;
                }

                // Own lifetime
                state->AddUser();

                immediateBatch.dirtyObjects.insert(state);
                immediateBatch.dirtyPipelines.push_back(state);
            }
            break;
        }

        case SetOrAddFilteredPipelineInstrumentationMessage::kID: {
            auto *message = it.Get<SetOrAddFilteredPipelineInstrumentationMessage>();

            // Try to find
            auto it = std::find_if(filteredInstrumentationInfo.begin(), filteredInstrumentationInfo.end(), [&](const FilterEntry& entry) {
                return entry.guid == message->guid.View();
            });

            // None?
            if (it == filteredInstrumentationInfo.end()) {
                it = filteredInstrumentationInfo.emplace(filteredInstrumentationInfo.end());

                // Copy properties
                it->guid = message->guid.View();
                it->name = message->name.View();
            }

            // Translate type
            switch (message->type) {
                default:
                    ASSERT(false, "Invalid pipeline type");
                    break;
                case 0:
                    it->type = PipelineType::Count;
                    break;
                case 1:
                    it->type = PipelineType::Graphics;
                    break;
                case 2:
                    it->type = PipelineType::Compute;
                    break;
            }

            // Copy info
            SetInstrumentationInfo(it->instrumentationInfo, message->featureBitSet, message->specialization);

            // Refilter all pipelines
            for (PipelineState *state: table->states_pipeline.GetLinear()) {
                if (immediateBatch.dirtyObjects.count(state) || !FilterPipeline(state, *it)) {
                    continue;
                }

                // Push pipeline
                immediateBatch.dirtyObjects.insert(state);
                immediateBatch.dirtyPipelines.push_back(state);

                // Own lifetime
                state->AddUser();

                // Push source modules
                for (ShaderModuleState *shaderState: state->shaderModules) {
                    if (immediateBatch.dirtyObjects.count(shaderState)) {
                        continue;
                    }

                    // Own lifetime
                    shaderState->AddUser();
                
                    immediateBatch.dirtyObjects.insert(shaderState);
                    immediateBatch.dirtyShaderModules.push_back(shaderState);
                }
            }
            break;
        }
    }

    // Flush
    virtualFeatureRedirects.clear();
}

void InstrumentationController::SetInstrumentationInfo(InstrumentationInfo &info, uint64_t bitSet, const MessageSubStream &stream) {
    // Set the enabled bit-set
    if (!virtualFeatureRedirects.empty()) {
        info.featureBitSet = 0x0;

        // Traverse bit set
        unsigned long index;
        while (_BitScanReverse64(&index, bitSet)) {
            // Translate bit set
            uint32_t physical = virtualFeatureRedirects[index];
            if (physical) {
                info.featureBitSet |= physical;
            } else {
                table->parent->logBuffer.Add("Vulkan", Format("Unknown virtual redirect at {}", index));
            }

            // Next!
            bitSet &= ~(1ull << index);
        }
    } else {
        // No virtualization, just inherit
        info.featureBitSet = bitSet;        
    }

    // Transfer sub stream
    stream.Transfer(info.specialization);
}

void InstrumentationController::CommitInstrumentation() {
    // Early out
    if (immediateBatch.dirtyObjects.empty()) {
        return;
    }
    
    compilationEvent.IncrementHead();

    // Diagnostic
#if LOG_INSTRUMENTATION
    table->parent->logBuffer.Add("Vulkan", Format(
        "Committing {} shaders and {} pipelines for instrumentation",
        immediateBatch.dirtyShaderModules.size(),
        immediateBatch.dirtyPipelines.size()
    ));
#endif // LOG_INSTRUMENTATION
    
    // Re-propagate all shaders
    for (ShaderModuleState* state : immediateBatch.dirtyShaderModules) {
        PropagateInstrumentationInfo(state);
    }

    // Re-propagate all pipelines
    for (PipelineState* state : immediateBatch.dirtyPipelines) {
        PropagateInstrumentationInfo(state);
    }

    // Copy batch
    auto* batch = new (registry->GetAllocators()) Batch(immediateBatch);
    batch->stampBegin = std::chrono::high_resolution_clock::now();

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

    // Get the current bucket
    compilationBucket = group.GetBucket();

    // Clean current batch
    immediateBatch.dirtyObjects.clear();
    immediateBatch.dirtyShaderModules.clear();
    immediateBatch.dirtyPipelines.clear();
}

void InstrumentationController::Commit() {
    uint32_t count = GetJobCount();

    // Any since last?
    if (lastPooledCount != count) {
        // Add diagnostic message
        auto message = MessageStreamView(commitStream).Add<JobDiagnosticMessage>();
        message->remaining = count;
        table->bridge->GetOutput()->AddStreamAndSwap(commitStream);

        // OK
        lastPooledCount = count;
    }

    // Commit all pending instrumentation
    std::lock_guard guard(mutex);
    CommitInstrumentation();
}

void InstrumentationController::CommitShaders(DispatcherBucket* bucket, void *data) {
    auto* batch = static_cast<Batch*>(data);
    batch->stampBeginShaders = std::chrono::high_resolution_clock::now();

    // Submit compiler jobs
    for (ShaderModuleState* state : batch->dirtyShaderModules) {
        uint64_t shaderFeatureBitSet = state->instrumentationInfo.featureBitSet;

        // Perform feedback from the dependent objects
        for (PipelineState* dependentObject : table->dependencies_shaderModulesPipelines.Get(state)) {
            // Get the super feature set
            uint64_t featureBitSet = shaderFeatureBitSet | dependentObject->instrumentationInfo.featureBitSet;

            // No features?
            if (!featureBitSet) {
                continue;
            }

            // Number of slots used by the pipeline
            uint32_t pipelineLayoutUserSlots = dependentObject->layout->boundUserDescriptorStates;

            // User push constant offset
            uint32_t pipelineLayoutDataPCOffset = dependentObject->layout->dataPushConstantOffset;
#if PRMT_METHOD == PRMT_METHOD_UB_PC
            uint32_t pipelineLayoutPRMTPCOffset = dependentObject->layout->prmtPushConstantOffset;
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC

            // Create the instrumentation key
            ShaderModuleInstrumentationKey instrumentationKey{};
            instrumentationKey.featureBitSet = featureBitSet;
            instrumentationKey.pipelineLayoutUserSlots = pipelineLayoutUserSlots;
            instrumentationKey.pipelineLayoutDataPCOffset = pipelineLayoutDataPCOffset;
#if PRMT_METHOD == PRMT_METHOD_UB_PC
            instrumentationKey.pipelineLayoutPRMTPCOffset = pipelineLayoutPRMTPCOffset;
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC

            // Attempt to reserve
            if (!state->Reserve(instrumentationKey)) {
                continue;
            }

            // Inject the feedback state
            shaderCompiler->Add(table, state, &batch->shaderCompilerDiagnostic, instrumentationKey, bucket);
        }
    }
}

void InstrumentationController::CommitPipelines(DispatcherBucket* bucket, void *data) {
    auto* batch = static_cast<Batch*>(data);
    batch->stampBeginPipelines = std::chrono::high_resolution_clock::now();

    // Collection of keys which failed
    std::vector<std::pair<ShaderModuleState*, ShaderModuleInstrumentationKey>> rejectedKeys;

    // Allocate batch
    auto jobs = new (registry->GetAllocators()) PipelineJob[batch->dirtyPipelines.size()];

    // Enqueued jobs
    uint32_t enqueuedJobs{0};

    // Submit compiler jobs
    for (size_t dirtyIndex = 0; dirtyIndex < batch->dirtyPipelines.size(); dirtyIndex++) {
        PipelineState *state = batch->dirtyPipelines[dirtyIndex];

        // Was this job skipped?
        bool isSkipped = false;

        // Setup the job
        PipelineJob& job = jobs[dirtyIndex];
        job.state = state;
        job.featureBitSet = state->instrumentationInfo.featureBitSet;

        // Allocate feature bit sets
        job.shaderModuleInstrumentationKeys = new (registry->GetAllocators()) ShaderModuleInstrumentationKey[state->shaderModules.size()];

        // Super set
        uint64_t superFeatureBitSet{0};

        // Set the module feature bit sets
        for (uint32_t shaderIndex = 0; shaderIndex < state->shaderModules.size(); shaderIndex++) {
            uint64_t featureBitSet = 0;

            // Create super feature bit set (shader -> pipeline)
            // ? Pipeline specific bit set fed back during shader compilation
            featureBitSet |= state->shaderModules[shaderIndex]->instrumentationInfo.featureBitSet;
            featureBitSet |= state->instrumentationInfo.featureBitSet;

            // Summarize
            superFeatureBitSet |= featureBitSet;

            // Number of slots used by the pipeline
            uint32_t pipelineLayoutUserSlots = state->layout->boundUserDescriptorStates;
            ASSERT(pipelineLayoutUserSlots <= table->physicalDeviceProperties.limits.maxBoundDescriptorSets, "Pipeline layout user slots sanity check failed (corrupt)");

            // User push constant offset
            uint32_t pipelineLayoutDataPCOffset = state->layout->dataPushConstantOffset;
#if PRMT_METHOD == PRMT_METHOD_UB_PC
            uint32_t pipelineLayoutPRMTPCOffset = state->layout->prmtPushConstantOffset;
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC

            // Create the instrumentation key
            ShaderModuleInstrumentationKey instrumentationKey{};
            instrumentationKey.featureBitSet = featureBitSet;
            instrumentationKey.pipelineLayoutUserSlots = pipelineLayoutUserSlots;
            instrumentationKey.pipelineLayoutDataPCOffset = pipelineLayoutDataPCOffset;
#if PRMT_METHOD == PRMT_METHOD_UB_PC
            instrumentationKey.pipelineLayoutPRMTPCOffset = pipelineLayoutPRMTPCOffset;
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC

            // Assign key
            job.shaderModuleInstrumentationKeys[shaderIndex] = instrumentationKey;

            // Shader may have failed to compile for whatever reason, skip if need be
            if (!job.state->shaderModules[shaderIndex]->HasInstrument(instrumentationKey)) {
                rejectedKeys.push_back(std::make_pair(job.state->shaderModules[shaderIndex], instrumentationKey));
                isSkipped = true;
            }
        }

        // No features?
        if (!superFeatureBitSet) {
            // Set the hot swapped object to native
            state->hotSwapObject.store(nullptr);
            isSkipped = true;
        }

        // Not of interest?
        if (isSkipped) {
            destroy(job.shaderModuleInstrumentationKeys, allocators);
            continue;
        }

        // Next job
        enqueuedJobs++;
    }

    // Submit all jobs
    pipelineCompiler->AddBatch(table, &batch->pipelineCompilerDiagnostic, jobs, enqueuedJobs, bucket);

    // Report all rejected keys
    if (!rejectedKeys.empty()) {
#if LOG_REJECTED_KEYS
        std::stringstream keyMessage;
        keyMessage << "Instrumentation failed for the following shaders and keys:\n";

        // Compose keys
        for (auto&& kv : rejectedKeys) {
            keyMessage << "\tShader " << kv.first->uid << " [" << kv.second.featureBitSet << "] with " << kv.second.pipelineLayoutUserSlots << " user slots\n";
        }

        // Submit
        table->parent->logBuffer.Add("Vulkan", keyMessage.str());
#endif // LOG_REJECTED_KEYS
    }

    // Free up
    destroy(jobs, registry->GetAllocators());
}

void InstrumentationController::CommitTable(DispatcherBucket* bucket, void *data) {
    auto* batch = static_cast<Batch*>(data);

    // Determine time difference
    auto msTotal = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() -  batch->stampBegin).count());
    auto msPipelines = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() -  batch->stampBeginPipelines).count());
    auto msShaders = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(batch->stampBeginPipelines -  batch->stampBeginShaders).count());

    // Commit all sguid changes
    auto bridge = registry->Get<IBridge>();
    table->sguidHost->Commit(bridge.GetUnsafe());

    // Set the enabled feature bit set
    SetDeviceCommandFeatureSetAndCommit(table, batch->featureBitSet);

    // Diagnostic
#if LOG_INSTRUMENTATION
    // Get failure counts
    const uint64_t failedShaders   = batch->shaderCompilerDiagnostic.failedJobs.load();
    const uint64_t failedPipelines = batch->pipelineCompilerDiagnostic.failedJobs.load();

    // Log on failure
    if (failedShaders || failedPipelines) {
        table->parent->logBuffer.Add("Vulkan", Format(
            "Instrumentation failed for {} shaders and {} pipelines",
            failedShaders,
            failedPipelines
        ));
    }
    
    table->parent->logBuffer.Add("Vulkan", Format(
        "Instrumented {} shaders ({} ms) and {} pipelines ({} ms), total {} ms",
        batch->dirtyShaderModules.size(),
        msShaders,
        batch->dirtyPipelines.size(),
        msPipelines,
        msTotal
    ));
#endif

    // Mark as done
    compilationEvent.IncrementCounter();

    // Release handles
    for (ReferenceObject* object : batch->dirtyObjects) {
        destroyRef(object, allocators);
    }

    // Release batch
    destroy(batch, allocators);

    // Release the bucket handle, destructed after this call 
    std::lock_guard guard(mutex);
    compilationBucket = nullptr;
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

void InstrumentationController::BeginCommandList() {
    // If synchronous, wait for the head compilation counter
    if (synchronousRecording) {
        compilationEvent.Wait(compilationEvent.GetHead());
    }
}
