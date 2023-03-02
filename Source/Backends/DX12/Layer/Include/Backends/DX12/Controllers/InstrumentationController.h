#pragma once

// Layer
#include <Backends/DX12/States/ShaderInstrumentationKey.h>
#include <Backends/DX12/Controllers/IController.h>
#include <Backends/DX12/InstrumentationInfo.h>

// Message
#include <Message/MessageStream.h>

// Common
#include <Common/Dispatcher/EventCounter.h>
#include <Common/ComRef.h>

// Bridge
#include <Bridge/IBridgeListener.h>

// Std
#include <vector>
#include <chrono>
#include <set>

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

    /// Invoked once a command list has begun recording
    void BeginCommandList();

    /// Commit all instrumentation changes
    void CommitInstrumentation();

    /// Commit all bridges
    void Commit();

    /// Get the number of jobs
    uint32_t GetJobCount();

protected:
    void CommitShaders(DispatcherBucket* bucket, void *data);
    void CommitPipelines(DispatcherBucket* bucket, void *data);
    void CommitTable(DispatcherBucket* bucket, void *data);

    /// Message handler
    void OnMessage(const ConstMessageStreamView<>::ConstIterator &it);

    /// Summarize the feature bit set used
    /// \return bit set
    uint64_t SummarizeFeatureBitSet();

    /// Set the instrumentation info
    /// \param info the destination info
    /// \param bitSet bit set of enabled features
    /// \param stream the specialization stream
    void SetInstrumentationInfo(InstrumentationInfo& info, uint64_t bitSet, const MessageSubStream& stream);

private:
    DeviceState* device;
    ComRef<ShaderCompiler> shaderCompiler;
    ComRef<PipelineCompiler> pipelineCompiler;
    ComRef<Dispatcher> dispatcher;

    /// The global info
    InstrumentationInfo globalInstrumentationInfo;

private:
    struct Batch {
        Batch(const Allocators& allocators) : dirtyShaders(allocators), dirtyPipelines(allocators) {
            
        }
        
        uint64_t featureBitSet;

        std::chrono::high_resolution_clock::time_point stampBegin;
        std::chrono::high_resolution_clock::time_point stampBeginShaders;
        std::chrono::high_resolution_clock::time_point stampBeginPipelines;

        std::set<ReferenceObject*> dirtyObjects;
        Vector<ShaderState*> dirtyShaders;
        Vector<PipelineState*> dirtyPipelines;
    };

    /// Dirty states
    Batch immediateBatch;

    /// Compilation event
    EventCounter compilationEvent;

private:
    /// Shared lock
    std::mutex mutex;

    /// Current compilation bucket, not thread safe
    DispatcherBucket* compilationBucket{nullptr};

    /// Shared bridge stream
    MessageStream commitStream;

    /// Last pooled job counter
    uint32_t lastPooledCount{0};

private:
    bool synchronousRecording{false};
};
