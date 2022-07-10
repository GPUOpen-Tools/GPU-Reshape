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
    InstrumentationController(DeviceState* device);

    /// Install the controller
    bool Install();

    /// Uninstall the controller
    void Uninstall();

    /// Overrides
    void Handle(const MessageStream *streams, uint32_t count) final;

    /// Invoked once a command buffer has begun recording
    void BeginCommandBuffer();

    /// Commit all changes
    void Commit();

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
        uint64_t featureBitSet;

        std::chrono::high_resolution_clock::time_point stamp;

        std::set<ReferenceObject*> dirtyObjects;
        std::vector<ShaderState*> dirtyShaders;
        std::vector<PipelineState*> dirtyPipelines;
    };

    /// Dirty states
    Batch immediateBatch;

    /// Compilation event
    EventCounter compilationEvent;

private:
    bool synchronousRecording{false};
};
