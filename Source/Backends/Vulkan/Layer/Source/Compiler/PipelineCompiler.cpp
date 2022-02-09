#include <Backends/Vulkan/Compiler/PipelineCompiler.h>
#include <Backends/Vulkan/States/ShaderModuleState.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>

// Common
#include "Common/Dispatcher/Dispatcher.h"
#include <Common/Registry.h>

PipelineCompiler::PipelineCompiler(DeviceDispatchTable *table) : table(table) {

}

bool PipelineCompiler::Install() {
    dispatcher = registry->Get<Dispatcher>();
    if (!dispatcher) {
        return false;
    }

    // OK
    return true;
}

void PipelineCompiler::AddBatch(DeviceDispatchTable *table, PipelineJob *jobs, uint32_t count, DispatcherBucket *bucket) {
    // Segment the types
    for (uint32_t i = 0; i < count; i++) {
        switch (jobs[i].state->type) {
            case PipelineType::Graphics:
                graphicsJobs.push_back(jobs[i]);
                break;
            case PipelineType::Compute:
                computeJobs.push_back(jobs[i]);
                break;
        }
    }

    AddBatchOfType(table, graphicsJobs, PipelineType::Graphics, bucket);
    AddBatchOfType(table, computeJobs, PipelineType::Compute, bucket);

    graphicsJobs.clear();
    computeJobs.clear();
}

void PipelineCompiler::AddBatchOfType(DeviceDispatchTable *table, const std::vector<PipelineJob> &jobs, PipelineType type, DispatcherBucket *bucket) {
    if (jobs.empty()) {
        return;
    }

    // The maximum number per batch
    //  Each compilation job is of varying time, so split up the batches across workers to ensure that they're spread out
    //  to whatever workers first take them
    constexpr uint32_t kMaxBatchSize = 64;

    // The maximum number of jobs per worker
    const uint32_t maxJobsPerWorker = static_cast<uint32_t>(jobs.size() / dispatcher->WorkerCount());

    // The ideal batch size, in case the batch size is small, take the max number of jobs per worker
    const uint32_t batchSize = std::max(1u, std::min(maxJobsPerWorker, kMaxBatchSize));

    // Create batches
    for (uint32_t i = 0; i < jobs.size(); i++) {
        const uint32_t count = std::min(static_cast<uint32_t>(jobs.size()) - i, batchSize);

        // Create a copy of the states
        auto copy = new(allocators) PipelineJob[count];
        std::memcpy(copy, &jobs[i], sizeof(PipelineJob) * count);

        // Create the job data
        auto data = new(registry->GetAllocators()) PipelineJobBatch{
            .table = table,
            .jobs = copy,
            .count = count
        };

        // Submit the job
        switch (type) {
            case PipelineType::Graphics:
                dispatcher->Add(BindDelegate(this, PipelineCompiler::WorkerGraphics), data, bucket);
                break;
            case PipelineType::Compute:
                dispatcher->Add(BindDelegate(this, PipelineCompiler::WorkerCompute), data, bucket);
                break;
        }
    }
}

void PipelineCompiler::WorkerGraphics(void *data) {
    auto *job = static_cast<PipelineJobBatch *>(data);
    CompileGraphics(*job);
    destroy(job, allocators);
}

void PipelineCompiler::WorkerCompute(void *data) {
    auto *job = static_cast<PipelineJobBatch *>(data);
    CompileCompute(*job);
    destroy(job, allocators);
}

void PipelineCompiler::CompileGraphics(const PipelineJobBatch &batch) {
    // Allocate creation info
    auto createInfos = ALLOCA_ARRAY(VkGraphicsPipelineCreateInfo, batch.count);

    // Get the number of stages
    uint32_t stageCount = 0;
    for (uint32_t i = 0; i < batch.count; i++) {
        PipelineJob &job = batch.jobs[i];
        stageCount += static_cast<uint32_t>(job.state->shaderModules.size());
    }

    // Allocate stage infos
    auto stageInfos = ALLOCA_ARRAY(VkPipelineShaderStageCreateInfo, stageCount);

    // Populate all creation infos
    for (uint32_t i = 0; i < batch.count; i++) {
        PipelineJob &job = batch.jobs[i];
        PipelineState *state = job.state;

        // Copy the deep creation info
        //  This is safe, as the change is done on the copy itself, the original deep copy is untouched
        ASSERT(state->type == PipelineType::Graphics, "Unexpected pipeline type");
        createInfos[i] = static_cast<GraphicsPipelineState *>(state)->createInfoDeepCopy.createInfo;

        // Set new instrumented stages
        for (uint32_t shaderIndex = 0; shaderIndex < state->shaderModules.size(); shaderIndex++) {
            ShaderModuleState *shaderState = state->shaderModules[shaderIndex];

            std::memcpy(&stageInfos[shaderIndex], &createInfos[i].pStages[shaderIndex], sizeof(VkPipelineShaderStageCreateInfo));
            stageInfos[i].module = shaderState->GetInstrument(job.shaderModuleInstrumentationKeys[shaderIndex]);
        }

        // Set new stage info
        createInfos[i].pStages = stageInfos;
        stageInfos += state->shaderModules.size();
    }

    // Created pipelines
    auto pipelines = ALLOCA_ARRAY(VkPipeline, batch.count);

    // TODO: Pipeline cache?
    VkResult result = batch.table->next_vkCreateGraphicsPipelines(batch.table->object, nullptr, batch.count, createInfos, nullptr, pipelines);
    if (result != VK_SUCCESS) {
        // TODO: Error reporting
        return;
    }

    // Set final objects
    for (uint32_t i = 0; i < batch.count; i++) {
        PipelineJob &job = batch.jobs[i];
        PipelineState *state = job.state;

        // Set the instrument for the given feature set
        state->AddInstrument(job.featureBitSet, pipelines[i]);

        // Set the hot swapped object
        state->hotSwapObject.store(pipelines[i]);
    }

    // Free bit sets
    for (uint32_t i = 0; i < batch.count; i++) {
        destroy(batch.jobs[i].shaderModuleInstrumentationKeys, allocators);
    }

    // Free job
    destroy(batch.jobs, allocators);
}

void PipelineCompiler::CompileCompute(const PipelineJobBatch &batch) {
    // Allocate creation info
    auto createInfos = ALLOCA_ARRAY(VkComputePipelineCreateInfo, batch.count);

    // Populate all creation infos
    for (uint32_t i = 0; i < batch.count; i++) {
        PipelineJob &job = batch.jobs[i];
        PipelineState *state = job.state;

        // Copy the deep creation info
        //  This is safe, as the change is done on the copy itself, the original deep copy is untouched
        ASSERT(state->type == PipelineType::Compute, "Unexpected pipeline type");
        createInfos[i] = static_cast<ComputePipelineState *>(state)->createInfoDeepCopy.createInfo;

        // Get the shader
        ASSERT(state->shaderModules.size() == 1, "Compute pipeline expected one shader module");
        ShaderModuleState *shaderState = state->shaderModules[0];

        // Assign instrumented version
        createInfos[i].stage.module = shaderState->GetInstrument(job.shaderModuleInstrumentationKeys[0]);
        ASSERT(createInfos[i].stage.module, "Invalid module");
    }

    // Created pipelines
    auto pipelines = ALLOCA_ARRAY(VkPipeline, batch.count);

    // TODO: Pipeline cache?
    VkResult result = batch.table->next_vkCreateComputePipelines(batch.table->object, nullptr, batch.count, createInfos, nullptr, pipelines);
    if (result != VK_SUCCESS) {
        // TODO: Error reporting
        return;
    }

    // Set final objects
    for (uint32_t i = 0; i < batch.count; i++) {
        PipelineJob &job = batch.jobs[i];
        PipelineState *state = job.state;

        // Set the instrument for the given feature set
        state->AddInstrument(job.featureBitSet, pipelines[i]);

        // Set the hot swapped object
        state->hotSwapObject.store(pipelines[i]);
    }

    // Free bit sets
    for (uint32_t i = 0; i < batch.count; i++) {
        destroy(batch.jobs[i].shaderModuleInstrumentationKeys, allocators);
    }

    // Free job
    destroy(batch.jobs, allocators);
}
