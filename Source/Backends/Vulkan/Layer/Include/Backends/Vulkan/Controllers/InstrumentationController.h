#pragma once

// Layer
#include <Backends/Vulkan/InstrumentationInfo.h>
#include <Backends/Vulkan/Controllers/IController.h>

// Common
#include <Common/EventCounter.h>

// Bridge
#include <Bridge/IBridgeListener.h>

// Std
#include <vector>
#include <set>

// Forward declarations
class Registry;
class Dispatcher;
class DispatcherBucket;
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

    bool Install();

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
    DeviceDispatchTable* table;
    ShaderCompiler* shaderCompiler;
    PipelineCompiler* pipelineCompiler;
    Dispatcher* dispatcher;

    /// The global info
    InstrumentationInfo globalInstrumentationInfo;

private:
    struct Batch {
        uint64_t featureBitSet;

        std::set<ReferenceObject*> dirtyObjects;
        std::vector<ShaderModuleState*> dirtyShaderModules;
        std::vector<PipelineState*> dirtyPipelines;
    };

    /// Dirty states
    Batch immediateBatch;

    /// Compilation event
    EventCounter compilationEvent;

private:
    bool synchronousRecording{false};
};
