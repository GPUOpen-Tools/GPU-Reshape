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

#pragma once

// Layer
#include <Backends/Vulkan/InstrumentationInfo.h>
#include <Backends/Vulkan/Controllers/IController.h>
#include <Backends/Vulkan/Controllers/InstrumentationStage.h>
#include <Backends/Vulkan/States/PipelineType.h>
#include <Backends/Vulkan/Compiler/Diagnostic/ShaderCompilerDiagnostic.h>
#include <Backends/Vulkan/Compiler/Diagnostic/PipelineCompilerDiagnostic.h>
#include <Backends/Vulkan/Compiler/Diagnostic/DiagnosticType.h>
#include <Backends/Vulkan/Compiler/PipelineCompiler.h>

// Common
#include <Common/Dispatcher/EventCounter.h>
#include <Common/Dispatcher/RelaxedAtomic.h>
#include <Common/Containers/Vector.h>
#include <Common/ComRef.h>

// Bridge
#include <Bridge/IBridgeListener.h>

// Std
#include <vector>
#include <chrono>
#include <set>
#include <unordered_map>

// Forward declarations
class Registry;
class Dispatcher;
struct DispatcherBucket;
struct DeviceDispatchTable;
struct MessageSubStream;
struct MessageStream;
struct PipelineState;
struct ShaderModuleState;
struct ReferenceObject;
class ShaderCompiler;
class PipelineCompiler;

class InstrumentationController final : public IController, public IBridgeListener {
public:
    InstrumentationController(DeviceDispatchTable* table);

    /// Install the controller
    bool Install();

    /// Uninstall the controller
    void Uninstall();

    /// Overrides
    void Handle(const MessageStream *streams, uint32_t count) final;

    /// Invoked once a command buffer has begun recording
    bool ConditionalWaitForCompletion() {
        // If synchronous, wait for the head compilation counter
        if (synchronousRecording) {
            WaitForCompletion();
        }

        // OK
        return synchronousRecording;
    }

    /// Commit all instrumentation changes
    void CommitInstrumentation();

    /// Commit all bridges changes
    void Commit();

    /// Wait for all outstanding jobs
    void WaitForCompletion();

    /// Get the number of jobs
    uint32_t GetJobCount();

public:
    /// Invoked on pipeline creation
    /// \param state given state
    void CreatePipeline(PipelineState* state);
    
    /// Invoked on pipeline creation
    /// Performs synchronized state addition
    /// \param state given state
    void CreatePipelineAndAdd(PipelineState* state);

protected:
    void CommitShaders(DispatcherBucket* bucket, void *data);
    void CommitPipelineLibraries(DispatcherBucket* bucket, void *data);
    void CommitPipelines(DispatcherBucket* bucket, void *data);
    void CommitOpaquePipelines(DispatcherBucket* bucket, PipelineState** pipelineStates, uint32_t count, InstrumentationStage stage, void *data);
    void CommitTable(DispatcherBucket* bucket, void *data);
    void CommitFeatureMessages();

    /// Message handler
    void OnMessage(const ConstMessageStreamView<>::ConstIterator &it);
    void OnStateRequest(const struct GetStateMessage &message);

    /// Summarize the feature bit set used
    /// \return bit set
    uint64_t SummarizeFeatureBitSet();

    /// Set the instrumentation info
    /// \param info the destination info
    /// \param bitSet bit set of enabled features
    /// \param stream the specialization stream
    void SetInstrumentationInfo(InstrumentationInfo& info, uint64_t bitSet, const MessageSubStream& stream);

    /// Propagate instrumentation states
    /// \param state destination pipeline
    void PropagateInstrumentationInfo(PipelineState* state);

    /// Propagate instrumentation states
    /// \param state destination shader
    void PropagateInstrumentationInfo(ShaderModuleState* state);

    /// Activate all relevant features and commit them
    /// \param featureBitSet enabled feature set
    /// \param previousFeatureBitSet previously enabled feature set
    void ActivateAndCommitFeatures(uint64_t featureBitSet, uint64_t previousFeatureBitSet);
    
private:
    struct FilterEntry {
        /// Assigned filter GUID
        std::string guid;

        /// Given pipeline type
        PipelineType type;

        /// Name subset
        std::string name;

        /// Desired instrumentation
        InstrumentationInfo instrumentationInfo;
    };

    /// Filter a pipeline against an entry
    /// \param state given pipeline
    /// \param filter tested filter
    /// \return true if passes
    bool FilterPipeline(PipelineState* state, const FilterEntry& filter);

    /// Invoked on pipeline creation
    /// \param state given state
    void CreatePipelineNoLock(PipelineState* state);

private:
    DeviceDispatchTable* table;
    ComRef<ShaderCompiler> shaderCompiler;
    ComRef<PipelineCompiler> pipelineCompiler;
    ComRef<Dispatcher> dispatcher;

private:
    /// The global info
    InstrumentationInfo globalInstrumentationInfo;

    /// Object specific instrumentation
    std::unordered_map<uint64_t, InstrumentationInfo> shaderUIDInstrumentationInfo;
    std::unordered_map<uint64_t, InstrumentationInfo> pipelineUIDInstrumentationInfo;

    /// Filtered instrumentation
    std::vector<FilterEntry> filteredInstrumentationInfo;

private:
    /// Virtual redirects, exists for a single session
    std::vector<uint32_t> virtualFeatureRedirects;

private:
    struct Batch {
        struct CommitEntry {
            /// Pending entry
            PipelineState* state{nullptr};

            /// Expected hash
            uint64_t combinedHash{0};
        };
        
        /// Given feature sets
        uint64_t previousFeatureBitSet{0};
        uint64_t featureBitSet{0};

        /// Current version id
        uint64_t versionID{0};

        /// Compiler diagnostics
        ShaderCompilerDiagnostic shaderCompilerDiagnostic;
        PipelineCompilerDiagnostic pipelineCompilerDiagnostic;

        /// All diagnostic messages
        DiagnosticBucket<DiagnosticType> messages;

        /// Stamps
        std::chrono::high_resolution_clock::time_point stampBegin;
        std::chrono::high_resolution_clock::time_point stampBeginShaders;
        std::chrono::high_resolution_clock::time_point stampBeginPipelines;

        /// All pending entries
        std::vector<CommitEntry> commitEntries;

        /// Dirty objects
        std::set<ReferenceObject*> dirtyObjects;
        std::vector<ShaderModuleState*> dirtyShaderModules;
        std::vector<PipelineState*> dirtyPipelineLibraries;
        std::vector<PipelineState*> dirtyPipelines;

        /// Current stage
        RelaxedAtomic<InstrumentationStage> stage{InstrumentationStage::None};

        // All stage counters
        RelaxedAtomic<uint32_t> stageCounters[static_cast<uint32_t>(PipelineType::Count)];

        // Threading bucket
        DispatcherBucket* bucket{nullptr};
    };

    /// Commit a pipeline for instrumentation
    /// \param batch parent batch
    /// \param state state being compiled
    /// \param dependentObject dependent state for the expected instrumentation keys
    /// \param jobs destination job queue
    /// \return success state
    bool CommitPipeline(Batch* batch, PipelineState* state, PipelineState* dependentObject, Vector<PipelineJob>& jobs);
    
    /// Dirty states
    Batch immediateBatch;

    /// Compilation event
    EventCounter compilationEvent;

private:
    /// Shared lock
    std::mutex mutex;

    /// Current compilation batch, not thread safe
    Batch* compilationBatch{nullptr};

    /// Shared bridge stream
    MessageStream commitStream;

    /// Last pooled job counter
    uint32_t lastPooledCount{0};

    /// Is a summarization pass pending?
    bool pendingResummarization{false};

    /// Pending compilation bucket?
    bool hasPendingBucket{false};

    /// Current version id
    uint64_t versionID{0};

private:
    /// The previous feature set during summarization
    uint64_t previousFeatureBitSet{0};

private:
    bool synchronousRecording{false};
};
