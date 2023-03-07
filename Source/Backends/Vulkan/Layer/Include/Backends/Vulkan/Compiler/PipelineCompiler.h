#pragma once

// Layer
#include <Backends/Vulkan/States/PipelineState.h>
#include <Backends/Vulkan/States/ShaderModuleInstrumentationKey.h>
#include <Backends/Vulkan/Compiler/PipelineCompilerDiagnostic.h>

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Std
#include <mutex>

class Dispatcher;
struct DispatcherBucket;
struct DeviceDispatchTable;

struct PipelineJob {
    PipelineState* state{ nullptr };

    /// TODO: Stack fallback
    ShaderModuleInstrumentationKey* shaderModuleInstrumentationKeys{nullptr};

    /// Pipeline specific feature bit set
    uint64_t featureBitSet{ 0 };
};

class PipelineCompiler : public TComponent<PipelineCompiler> {
public:
    COMPONENT(PipelineCompiler);

    PipelineCompiler(DeviceDispatchTable* table);

    /// Install this compiler
    bool Install();

    /// Add a pipeline batch job
    /// \param states all pipeline states
    /// \param diagnostic pipeline diagnostic
    /// \param count the number of states
    /// \param bucket optional, dispatcher bucket
    void AddBatch(DeviceDispatchTable* table, PipelineCompilerDiagnostic* diagnostic, PipelineJob* jobs, uint32_t count, DispatcherBucket* bucket = nullptr);

protected:
    struct PipelineJobBatch {
        DeviceDispatchTable*        table;
        PipelineCompilerDiagnostic* diagnostic;
        PipelineJob*                jobs;
        uint32_t                    count;
    };

    /// Add a pipeline batch job
    /// \param table parent table
    /// \param diagnostic pipeline diagnostic
    /// \param jobs all pipeline jobs
    /// \param count the number of states
    /// \param bucket optional, dispatcher bucket
    void AddBatchOfType(DeviceDispatchTable* table, PipelineCompilerDiagnostic* diagnostic, const std::vector<PipelineJob>& jobs, PipelineType type, DispatcherBucket* bucket);

    /// Compile a given job
    void CompileGraphics(const PipelineJobBatch& job);
    void CompileCompute(const PipelineJobBatch& job);

    /// Worker entry
    void WorkerGraphics(void* userData);
    void WorkerCompute(void* userData);

private:
    DeviceDispatchTable* table{nullptr};

    /// Job buckets
    std::vector<PipelineJob> graphicsJobs;
    std::vector<PipelineJob> computeJobs;

    /// Async dispatcher
    ComRef<Dispatcher> dispatcher{nullptr};
};
