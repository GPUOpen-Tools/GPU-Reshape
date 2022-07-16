#pragma once

// Layer
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/States/ShaderInstrumentationKey.h>

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Std
#include <mutex>

// Forward declarations
class Dispatcher;
struct DispatcherBucket;

struct PipelineJob {
    /// Parent state
    PipelineState* state{ nullptr };

    /// TODO: Stack fallback
    ShaderInstrumentationKey* shaderInstrumentationKeys{nullptr};

    /// Pipeline specific feature bit set
    uint64_t featureBitSet{ 0 };
};

class PipelineCompiler : public TComponent<PipelineCompiler> {
public:
    COMPONENT(PipelineCompiler);

    PipelineCompiler(DeviceState *device);

    /// Install this compiler
    bool Install();

    /// Add a pipeline batch job
    /// \param states all pipeline states
    /// \param count the number of states
    /// \param bucket optional, dispatcher bucket
    void AddBatch(PipelineJob* jobs, uint32_t count, DispatcherBucket* bucket = nullptr);

protected:
    struct PipelineJobBatch {
        PipelineJob*         jobs;
        uint32_t             count;
    };

    /// Add a pipeline batch job
    /// \param states all pipeline states
    /// \param count the number of states
    /// \param bucket optional, dispatcher bucket
    void AddBatchOfType(const std::vector<PipelineJob>& jobs, PipelineType type, DispatcherBucket* bucket);

    /// Compile a given job
    void CompileGraphics(const PipelineJobBatch& job);
    void CompileCompute(const PipelineJobBatch& job);

    /// Worker entry
    void WorkerGraphics(void* userData);
    void WorkerCompute(void* userData);

private:
    DeviceState *device;

    /// Job buckets
    std::vector<PipelineJob> graphicsJobs;
    std::vector<PipelineJob> computeJobs;

    /// Async dispatcher
    ComRef<Dispatcher> dispatcher{nullptr};
};