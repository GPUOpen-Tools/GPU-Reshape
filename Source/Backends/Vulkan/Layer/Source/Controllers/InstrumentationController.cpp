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
#include <Backends/Vulkan/Compiler/Diagnostic/DiagnosticPrettyPrint.h>

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
#include <Common/Hash.h>

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
    
    // No compiling batch?
    if (!compilationBatch) {
        return 0u;
    }

    // Get number of jobs in the stage
    switch (compilationBatch->stage.load()) {
        default:
            return 0u;
        case InstrumentationStage::Shaders:
            return static_cast<uint32_t>(compilationBatch->shaderCompilerDiagnostic.GetRemainingJobs());
        case InstrumentationStage::Pipelines:
            return static_cast<uint32_t>(compilationBatch->pipelineCompilerDiagnostic.GetRemainingJobs());
    }
}

void InstrumentationController::CreatePipeline(PipelineState *state) {
    std::lock_guard guard(mutex);

    // Pass down
    CreatePipelineNoLock(state);
}

void InstrumentationController::CreatePipelineAndAdd(PipelineState *state) {
    std::lock_guard guard(mutex);

    // Pass down
    CreatePipelineNoLock(state);

    // Add dependencies, shader module -> pipeline
    for (ShaderModuleState * shader : state->shaderModules) {
        table->dependencies_shaderModulesPipelines.Add(shader, state);
    }
    
    // Add to state
    // ! This is a double lock, however, the lock hierarchy should be the same
    table->states_pipeline.Add(state->object, state);
}

void InstrumentationController::CreatePipelineNoLock(PipelineState *state) {
    // Mark as pending
    pendingResummarization = true;
    
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

    // Compute hash
    state->instrumentationInfo.specializationHash = BufferCRC32Short(
        state->instrumentationInfo.specialization.GetDataBegin(),
        state->instrumentationInfo.specialization.GetByteSize()
    );

    // Clean previous dependent streams
    state->dependentInstrumentationInfo.specializations.resize(state->shaderModules.size());
    for (MessageStream& dependentStream : state->dependentInstrumentationInfo.specializations) {
        dependentStream.ClearWithSchemaInvalidate();
    }

    // Summarize dependent specialization states
    for (size_t i = 0; i < state->shaderModules.size(); i++) {
        ShaderModuleState *dependentState = state->shaderModules[i];
        
        // Nothing to enqueue?
        if (state->instrumentationInfo.specialization.IsEmpty() && dependentState->instrumentationInfo.specialization.IsEmpty()) {
            continue;
        }

        // Summarize the shader specialization followed by pipeline specialization
        MessageStream& stream = state->dependentInstrumentationInfo.specializations[i];
        stream.Append(dependentState->instrumentationInfo.specialization);
        stream.Append(state->instrumentationInfo.specialization);
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

    // Compute hash
    state->instrumentationInfo.specializationHash = BufferCRC32Short(
        state->instrumentationInfo.specialization.GetDataBegin(),
        state->instrumentationInfo.specialization.GetByteSize()
    );
}

void InstrumentationController::ActivateAndCommitFeatures(uint64_t featureBitSet, uint64_t previousFeatureBitSet) {
    // Set the enabled feature bit set
    SetDeviceCommandFeatureSetAndCommit(table, featureBitSet);
    
    // Feature events
    for (size_t i = 0; i < table->features.size(); i++) {
        uint64_t bit = 1ull << i;;

        // Inform activation, state-less
        if (featureBitSet & bit) {
            table->features[i]->Activate(FeatureActivationStage::Commit);
        }

        // Inform feature deactivation
        if (!(featureBitSet & bit) && (previousFeatureBitSet & bit)) {
            table->features[i]->Deactivate();
        }
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

    // Flush redirects
    virtualFeatureRedirects.clear();

    CommitInstrumentation();
}

void InstrumentationController::OnMessage(const ConstMessageStreamView<>::ConstIterator &it) {
    switch (it.GetID()) {
        // Config
        case PauseInstrumentationMessage::kID: {
            dispatcher->SetPaused(it.Get<PauseInstrumentationMessage>()->paused);
            break;
        }
        case SetApplicationInstrumentationConfigMessage::kID: {
            auto *message = it.Get<SetApplicationInstrumentationConfigMessage>();
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
                table->parent->logBuffer.Add("Vulkan", LogSeverity::Error, Format("Virtual redirect failed for feature \"{}\"", message->name.View()));
            }
            break;
        }

        // Global instrumentation
        case SetGlobalInstrumentationMessage::kID: {
            auto *message = it.Get<SetGlobalInstrumentationMessage>();

            // Mark as pending
            pendingResummarization = true;

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

            // Mark as pending
            pendingResummarization = true;

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

            // Mark as pending
            pendingResummarization = true;

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
            auto *message = it.Get<RemoveFilteredPipelineInstrumentationMessage>();

            // Mark as pending
            pendingResummarization = true;

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

            // Mark as pending
            pendingResummarization = true;

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

        /** States */
        case GetStateMessage::kID: {
            OnStateRequest(*it.Get<GetStateMessage>());
            break;
        }
    }
}

void InstrumentationController::OnStateRequest(const struct GetStateMessage &message) {
    switch (message.uuid) {
        default:
            break;
        case SetGlobalInstrumentationMessage::kID: {
            if (globalInstrumentationInfo.featureBitSet) {
                auto response = MessageStreamView(commitStream).Add<SetGlobalInstrumentationMessage>();
                response->featureBitSet = globalInstrumentationInfo.featureBitSet;
            }
            break;
        }
        case SetShaderInstrumentationMessage::kID: {
            for (auto && shaderUIDInfo : shaderUIDInstrumentationInfo) {
                auto response = MessageStreamView(commitStream).Add<SetShaderInstrumentationMessage>();
                response->shaderUID = shaderUIDInfo.first;
                response->featureBitSet = shaderUIDInfo.second.featureBitSet;
            }
            break;
        }
        case SetPipelineInstrumentationMessage::kID: {
            for (auto && pipelineUIDInfo : pipelineUIDInstrumentationInfo) {
                auto response = MessageStreamView(commitStream).Add<SetPipelineInstrumentationMessage>();
                response->pipelineUID = pipelineUIDInfo.first;
                response->featureBitSet = pipelineUIDInfo.second.featureBitSet;
            }
            break;
        }
        case SetOrAddFilteredPipelineInstrumentationMessage::kID: {
            for (auto && filter : filteredInstrumentationInfo) {
                auto response = MessageStreamView(commitStream).Add<SetOrAddFilteredPipelineInstrumentationMessage>(SetOrAddFilteredPipelineInstrumentationMessage::AllocationInfo{
                    .guidLength = filter.guid.length(),
                    .nameLength = filter.name.length()
                });
                response->guid.Set(filter.guid);
                response->name.Set(filter.name);
                response->type = static_cast<uint32_t>(filter.type);
                response->featureBitSet = filter.instrumentationInfo.featureBitSet;

                // Translate type
                switch (filter.type) {
                    default:
                        response->type = 0;
                        break;
                    case PipelineType::Graphics:
                        response->type = 1;
                        break;
                    case PipelineType::Compute:
                        response->type = 2;
                        break;
                }
            }
            break;
        }
    }
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
                table->parent->logBuffer.Add("Vulkan", LogSeverity::Error, Format("Unknown virtual redirect at {}", index));
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
    uint64_t featureBitSet = 0;
    
    // Early out
    if (immediateBatch.dirtyObjects.empty() && !pendingResummarization) {
        return;
    }

    // Wait for pending buckets, compilation has to maintain ordering
    if (compilationBatch) {
        // Increment counter, pending work may require synchronous recording.
        // If said work is scheduled in before the bucket is done compiling, and the counter has
        // not been incremented, due to ordering, it may start ahead of schedule.
        if (!immediateBatch.dirtyObjects.empty() && !hasPendingBucket) {
            compilationEvent.IncrementHead();
            hasPendingBucket = true;
        }

        // Skip
        return;
    }
    
    // Re-propagate all shaders
    for (ShaderModuleState* state : immediateBatch.dirtyShaderModules) {
        PropagateInstrumentationInfo(state);
    }

    // Re-propagate all pipelines
    for (PipelineState* state : immediateBatch.dirtyPipelines) {
        PropagateInstrumentationInfo(state);
    }
    
    // Resummarize and commit the feature set if needed
    if (pendingResummarization) {
        featureBitSet = SummarizeFeatureBitSet();

        // Mark as summarized
        pendingResummarization = false;
    }
    
    // If no dirty objects, nothing to instrument
    if (immediateBatch.dirtyObjects.empty()) {
        // Nothing to instrument, activate the features as "instrumented"
        ActivateAndCommitFeatures(featureBitSet, previousFeatureBitSet);
        previousFeatureBitSet = featureBitSet;
        return;
    }

    // Mark next batch
    if (!hasPendingBucket) {
        compilationEvent.IncrementHead();
    }

    // Diagnostic
#if LOG_INSTRUMENTATION
    table->parent->logBuffer.Add("Vulkan", LogSeverity::Info, Format(
        "Committing {} unique shaders and {} pipelines for instrumentation",
        immediateBatch.dirtyShaderModules.size(),
        immediateBatch.dirtyPipelines.size()
    ));
#endif // LOG_INSTRUMENTATION

    // Copy batch
    auto* batch = new (registry->GetAllocators()) Batch(immediateBatch);
    batch->stampBegin = std::chrono::high_resolution_clock::now();

    // Summarize the needed feature set
    batch->previousFeatureBitSet = previousFeatureBitSet;
    batch->featureBitSet = featureBitSet;

    // Inform activation, state-less
    for (size_t i = 0; i < table->features.size(); i++) {
        if (batch->featureBitSet & (1ull << i)) {
            table->features[i]->Activate(FeatureActivationStage::Instrumentation);
        }
    }

    // Keep track of last bit set
    previousFeatureBitSet = featureBitSet;

    // Task group
    // TODO: Tie lifetime of this task group to the controller
    TaskGroup group(dispatcher.GetUnsafe());
    if (!batch->dirtyShaderModules.empty()) {
        group.Chain(BindDelegate(this, InstrumentationController::CommitShaders), batch);
    }
    if (!batch->dirtyPipelines.empty()) {
        group.Chain(BindDelegate(this, InstrumentationController::CommitPipelines), batch);
    }
    group.Chain(BindDelegate(this, InstrumentationController::CommitTable), batch);

    // Start the group
    group.Commit();

    // Keep the bucket for tracking
    batch->bucket = group.GetBucket();

    // Set the current batch
    compilationBatch = batch;

    // Clean current batch
    immediateBatch.dirtyObjects.clear();
    immediateBatch.dirtyShaderModules.clear();
    immediateBatch.dirtyPipelines.clear();

    // Cleanup
    hasPendingBucket = false;
}

void InstrumentationController::CommitFeatureMessages() {
    // Always commit the sguid host before,
    // since this may be collected during instrumentation
    table->sguidHost->Commit(table->bridge.GetUnsafe());
    
    // Commit all feature messages
    for (const ComRef<IFeature>& feature : table->features) {
        feature->CollectMessages(table->bridge->GetOutput());
    }
}

void InstrumentationController::Commit() {
    uint32_t count = GetJobCount();

    // Commit all collected feature messages
    CommitFeatureMessages();

    // Serial
    std::lock_guard guard(mutex);

    // Any since last?
    if (lastPooledCount != count) {
        // Add diagnostic message
        auto message = MessageStreamView(commitStream).Add<JobDiagnosticMessage>();
        message->remaining = count;

        // Set batch stage information
        if (compilationBatch) {
            message->stage = static_cast<uint32_t>(compilationBatch->stage.load());
            message->graphicsJobs = compilationBatch->stageCounters[static_cast<uint32_t>(PipelineType::Graphics)].load();
            message->computeJobs = compilationBatch->stageCounters[static_cast<uint32_t>(PipelineType::Compute)].load();
        } else {
            message->stage = 0;
            message->graphicsJobs = 0;
            message->computeJobs = 0;
        }
        
        // OK
        lastPooledCount = count;
    }

    // Anything to commit?
    if (!commitStream.IsEmpty()) {
        table->bridge->GetOutput()->AddStreamAndSwap(commitStream);
    }

    // Commit all pending instrumentation
    CommitInstrumentation();
}

void InstrumentationController::CommitShaders(DispatcherBucket* bucket, void *data) {
    auto* batch = static_cast<Batch*>(data);
    batch->stampBeginShaders = std::chrono::high_resolution_clock::now();

    // Configure
    batch->stage = InstrumentationStage::Shaders;
    batch->shaderCompilerDiagnostic.messages = &batch->messages;

    // Reset counters
    std::fill_n(batch->stageCounters, static_cast<uint32_t>(PipelineType::Count), 0u);

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

            // Raytracing is pass-through for now
            if (dependentObject->type == PipelineType::Raytracing) {
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
            instrumentationKey.physicalMapping = &dependentObject->layout->physicalMapping;

            // Combine hashes
            instrumentationKey.combinedHash = dependentObject->instrumentationInfo.specializationHash;
            CombineHash(instrumentationKey.combinedHash, state->instrumentationInfo.specializationHash);
            CombineHash(instrumentationKey.combinedHash, instrumentationKey.pipelineLayoutUserSlots);
            CombineHash(instrumentationKey.combinedHash, instrumentationKey.pipelineLayoutDataPCOffset);
#if PRMT_METHOD == PRMT_METHOD_UB_PC
            CombineHash(instrumentationKey.combinedHash, instrumentationKey.pipelineLayoutPRMTPCOffset);
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC
            CombineHash(instrumentationKey.combinedHash, instrumentationKey.physicalMapping->layoutHash);

            // Attempt to reserve
            if (!state->Reserve(instrumentationKey)) {
                continue;
            }

            // Increment counter
            batch->stageCounters[static_cast<uint32_t>(dependentObject->type)]++;

            // Determine the shader module index within the dependent object
            uint64_t dependentIndex = std::ranges::find(dependentObject->shaderModules, state) - dependentObject->shaderModules.begin();

            // Inject the feedback state
            shaderCompiler->Add(table, ShaderJob {
                .state = state,
                .instrumentationKey = instrumentationKey,
                .diagnostic = &batch->shaderCompilerDiagnostic,
                .dependentSpecialization = &dependentObject->dependentInstrumentationInfo.specializations[dependentIndex]
            }, bucket);
        }
    }
}

void InstrumentationController::CommitPipelines(DispatcherBucket* bucket, void *data) {
    auto* batch = static_cast<Batch*>(data);
    batch->stampBeginPipelines = std::chrono::high_resolution_clock::now();

    // Set configure
    batch->stage = InstrumentationStage::Pipelines;
    batch->pipelineCompilerDiagnostic.messages = &batch->messages;

    // Reset counters
    std::fill_n(batch->stageCounters, static_cast<uint32_t>(PipelineType::Count), 0u);

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
        PipelineJob& job = jobs[enqueuedJobs];
        job.state = state;
        job.combinedHash = 0x0;

        // Allocate feature bit sets
        job.shaderModuleInstrumentationKeys = new (registry->GetAllocators()) ShaderModuleInstrumentationKey[state->shaderModules.size()];

        // Super set
        uint64_t superFeatureBitSet{0};

        // Set the module feature bit sets
        for (uint32_t shaderIndex = 0; shaderIndex < state->shaderModules.size(); shaderIndex++) {
            uint64_t featureBitSet = 0;

            // Get shader
            ShaderModuleState* shaderState = state->shaderModules[shaderIndex];

            // Create super feature bit set (shader -> pipeline)
            // ? Pipeline specific bit set fed back during shader compilation
            featureBitSet |= shaderState->instrumentationInfo.featureBitSet;
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
            instrumentationKey.physicalMapping = &state->layout->physicalMapping;

            // Combine hashes
            instrumentationKey.combinedHash = state->instrumentationInfo.specializationHash;
            CombineHash(instrumentationKey.combinedHash, shaderState->instrumentationInfo.specializationHash);
            CombineHash(instrumentationKey.combinedHash, instrumentationKey.pipelineLayoutUserSlots);
            CombineHash(instrumentationKey.combinedHash, instrumentationKey.pipelineLayoutDataPCOffset);
#if PRMT_METHOD == PRMT_METHOD_UB_PC
            CombineHash(instrumentationKey.combinedHash, instrumentationKey.pipelineLayoutPRMTPCOffset);
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC
            CombineHash(instrumentationKey.combinedHash, instrumentationKey.physicalMapping->layoutHash);

            // Assign key
            job.shaderModuleInstrumentationKeys[shaderIndex] = instrumentationKey;

            // Combine parent hash
            CombineHash(job.combinedHash, instrumentationKey.combinedHash);
            
            // Shader may have failed to compile for whatever reason, skip if need be
            if (!shaderState->HasInstrument(instrumentationKey)) {
                rejectedKeys.push_back(std::make_pair(shaderState, instrumentationKey));
                isSkipped = true;
            }
        }

        // Raytracing is pass-through for now
        if (state->type == PipelineType::Raytracing) {
            superFeatureBitSet = 0x0;
        }

        // No features?
        if (!superFeatureBitSet) {
            // Set the hot swapped object to native
            state->hotSwapObject.store(nullptr);
            isSkipped = true;
        }

        // Not of interest?
        if (isSkipped) {
            // May be empty
            if (!state->shaderModules.empty()) {
                destroy(job.shaderModuleInstrumentationKeys, allocators);
            }
            
            continue;
        }

        // Increment counter
        batch->stageCounters[static_cast<uint32_t>(state->type)]++;

        // Append commit entry
        batch->commitEntries.push_back(Batch::CommitEntry {
            .state = state,
            .combinedHash = job.combinedHash,
        });

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
        table->parent->logBuffer.Add("Vulkan", LogSeverity::Error, keyMessage.str());
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

    // Activate the features
    ActivateAndCommitFeatures(batch->featureBitSet, batch->previousFeatureBitSet);

    // Commit all pending entries
    for (Batch::CommitEntry entry : batch->commitEntries) {
        if (auto pipeline = entry.state->GetInstrument(entry.combinedHash)) {
            entry.state->hotSwapObject.store(pipeline);
        }
    }

    // Sync scope
    {
        std::lock_guard guard(mutex);
        
        // Diagnostic
#if LOG_INSTRUMENTATION
        // Create diagnostic stream
        MessageStream diagnosticStream;
        {
            // Shared buffer
            std::stringstream stream;

            // Append view
            MessageStreamView<CompilationDiagnosticMessage> view(diagnosticStream);

            // Translate all messages
            for (const DiagnosticMessage<DiagnosticType>& message : batch->messages) {
                DiagnosticPrettyPrint(message, stream);
                
                // Create message
                stream.seekg(0, std::ios_base::end);
                auto&& diag = view.Add(CompilationDiagnosticMessage::AllocationInfo {
                    .contentLength = static_cast<uint32_t>(stream.tellg())
                });

                // Copy contents
                stream.seekg(0, std::ios_base::beg);
                diag->content.Set(stream.view());
                stream.str(std::string());
            }
        }

        // Create final diagnostics message
        auto message = MessageStreamView(commitStream).Add<InstrumentationDiagnosticMessage>(InstrumentationDiagnosticMessage::AllocationInfo {
            .messagesByteSize = diagnosticStream.GetByteSize()
        });

        // Fill message
        message->messages.Set(diagnosticStream);
        message->passedShaders = static_cast<uint32_t>(batch->shaderCompilerDiagnostic.passedJobs.load());
        message->failedShaders = static_cast<uint32_t>(batch->shaderCompilerDiagnostic.failedJobs.load());
        message->passedPipelines = static_cast<uint32_t>(batch->pipelineCompilerDiagnostic.passedJobs.load());
        message->failedPipelines = static_cast<uint32_t>(batch->pipelineCompilerDiagnostic.failedJobs.load());
        message->millisecondsShaders = msShaders;
        message->millisecondsPipelines = msPipelines;
        message->millisecondsTotal = msTotal;
#endif // LOG_INSTRUMENTATION

        // Release the batch, bucket destructed after this call
        compilationBatch = nullptr;

        // Mark as done
        compilationEvent.IncrementCounter();

        // Recommit the immediate batch
        // Previous commits may be held up since they're waiting on the current batch to complete,
        // While we could have a separate thread to track all of this, it's better if it can be
        // handled with the applications threading alone.
        CommitInstrumentation();
    }

    // Release handles
    for (ReferenceObject* object : batch->dirtyObjects) {
        destroyRef(object, allocators);
    }

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

void InstrumentationController::WaitForCompletion() {
    // Commit all pending instrumentation
    uint64_t headCounter;
    {
        std::lock_guard guard(mutex);
        CommitInstrumentation();

        // Only get head inside guard, otherwise there's a risk its incremented without any
        // way to commit it in the future.
        headCounter = compilationEvent.GetHead();
    }

    // Wait til head
    compilationEvent.Wait(headCounter);
}
