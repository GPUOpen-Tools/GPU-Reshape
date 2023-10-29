// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
#include <Backends/DX12/States/ShaderInstrumentationKey.h>
#include <Backends/DX12/Controllers/IController.h>
#include <Backends/DX12/Controllers/InstrumentationStage.h>
#include <Backends/DX12/InstrumentationInfo.h>
#include <Backends/DX12/States/PipelineType.h>
#include <Backends/DX12/Compiler/Diagnostic/ShaderCompilerDiagnostic.h>
#include <Backends/DX12/Compiler/Diagnostic/PipelineCompilerDiagnostic.h>
#include <Backends/DX12/Compiler/Diagnostic/DiagnosticType.h>

// Message
#include <Message/MessageStream.h>

// Common
#include <Common/Dispatcher/EventCounter.h>
#include <Common/Dispatcher/RelaxedAtomic.h>
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
struct DeviceState;
struct MessageSubStream;
struct MessageStream;
struct PipelineState;
struct ShaderState;
struct ReferenceObject;
class ShaderCompiler;
class PipelineCompiler;

class InstrumentationController final : public IController, public IBridgeListener {
public:
    COMPONENT(InstrumentationController);

    InstrumentationController(DeviceState* device);

    /// Install the controller
    bool Install();

    /// Uninstall the controller
    void Uninstall();

    /// Overrides
    void Handle(const MessageStream *streams, uint32_t count) final;

    /// Commit all instrumentation changes
    void CommitInstrumentation();

    /// Commit all bridges
    void Commit();

    /// Wait for all outstanding jobs
    void WaitForCompletion();

    /// Wait for all outstanding jobs if the instrumentation configuration dictates it
    bool ConditionalWaitForCompletion() {
        // If synchronous, wait for the head compilation counter
        if (synchronousRecording) {
            WaitForCompletion();
        }

        // OK
        return synchronousRecording;
    }

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
    void CommitPipelines(DispatcherBucket* bucket, void *data);
    void CommitTable(DispatcherBucket* bucket, void *data);

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
    void PropagateInstrumentationInfo(ShaderState* state);

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
    DeviceState* device;
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
    Vector<FilterEntry> filteredInstrumentationInfo;

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
        
        Batch(const Allocators& allocators) : commitEntries(allocators), dirtyShaders(allocators), dirtyPipelines(allocators) {
            
        }

        /// Given feature set
        uint64_t featureBitSet;

        /// Compiler diagnostics
        ShaderCompilerDiagnostic shaderCompilerDiagnostic;
        PipelineCompilerDiagnostic pipelineCompilerDiagnostic;

        /// All diagnostic messages
        DiagnosticBucket<DiagnosticType> messages;
        
        /// All pending entries
        Vector<CommitEntry> commitEntries;

        /// Time stamps
        std::chrono::high_resolution_clock::time_point stampBegin;
        std::chrono::high_resolution_clock::time_point stampBeginShaders;
        std::chrono::high_resolution_clock::time_point stampBeginPipelines;

        /// All dirty objects
        std::set<ReferenceObject*> dirtyObjects;
        Vector<ShaderState*> dirtyShaders;
        Vector<PipelineState*> dirtyPipelines;

        /// Current stage
        RelaxedAtomic<InstrumentationStage> stage{InstrumentationStage::None};

        // All stage counters
        RelaxedAtomic<uint32_t> stageCounters[static_cast<uint32_t>(PipelineType::Count)];

        // Threading bucket
        DispatcherBucket* bucket{nullptr};
    };

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

private:
    bool synchronousRecording{false};
};
