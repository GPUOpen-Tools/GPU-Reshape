#pragma once

// Layer
#include <Backends/Vulkan/InstrumentationInfo.h>
#include <Backends/Vulkan/Controllers/IController.h>

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
    void BeginCommandList();

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
    ComRef<ShaderCompiler> shaderCompiler;
    ComRef<PipelineCompiler> pipelineCompiler;
    ComRef<Dispatcher> dispatcher;

    /// The global info
    InstrumentationInfo globalInstrumentationInfo;

private:
    struct Batch {
        uint64_t featureBitSet;

        std::chrono::high_resolution_clock::time_point stampBegin;
        std::chrono::high_resolution_clock::time_point stampBeginShaders;
        std::chrono::high_resolution_clock::time_point stampBeginPipelines;

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
