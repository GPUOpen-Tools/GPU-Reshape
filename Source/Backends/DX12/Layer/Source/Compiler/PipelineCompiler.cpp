#include <Backends/DX12/Compiler/PipelineCompiler.h>
#include <Backends/DX12/States/ShaderState.h>
#include <Backends/DX12/States/DeviceState.h>

// Common
#include <Common/Dispatcher/Dispatcher.h>
#include <Common/Registry.h>

// TODO: Fix this pollution
#undef min
#undef max

PipelineCompiler::PipelineCompiler(DeviceState *device) : device(device) {

}

bool PipelineCompiler::Install() {
    dispatcher = registry->Get<Dispatcher>();
    if (!dispatcher) {
        return false;
    }

    // OK
    return true;
}

void PipelineCompiler::AddBatch(PipelineJob *jobs, uint32_t count, DispatcherBucket *bucket) {
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

    AddBatchOfType(graphicsJobs, PipelineType::Graphics, bucket);
    AddBatchOfType(computeJobs, PipelineType::Compute, bucket);

    graphicsJobs.clear();
    computeJobs.clear();
}

void PipelineCompiler::AddBatchOfType(const std::vector<PipelineJob> &jobs, PipelineType type, DispatcherBucket *bucket) {
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
    for (uint32_t offset = 0; offset < jobs.size();) {
        const uint32_t count = std::min(static_cast<uint32_t>(jobs.size()) - offset, batchSize);

        // Create a copy of the states
        auto copy = new(allocators) PipelineJob[count];
        std::memcpy(copy, &jobs[offset], sizeof(PipelineJob) * count);

        // Create the job data
        auto data = new(registry->GetAllocators()) PipelineJobBatch{
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

        // Next!
        offset += count;
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
    // Populate all creation infos
    for (uint32_t i = 0; i < batch.count; i++) {
        PipelineJob &job = batch.jobs[i];
        PipelineState *state = job.state;

        // State
        auto graphicsState = static_cast<GraphicsPipelineState *>(state);

        // Copy the deep creation info
        //  This is safe, as the change is done on the copy itself, the original deep copy is untouched
        ASSERT(state->type == PipelineType::Graphics, "Unexpected pipeline type");
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = graphicsState->deepCopy.desc;

        // Instrumentation offset
        uint32_t keyOffset{0};

        // Vertex shader
        if (graphicsState->vs) {
            desc.VS = graphicsState->vs->GetInstrument(job.shaderInstrumentationKeys[keyOffset++]);
            if (!desc.VS.pShaderBytecode) {
                continue;
            }
        }

        // Hull shader
        if (graphicsState->hs) {
            desc.HS = graphicsState->hs->GetInstrument(job.shaderInstrumentationKeys[keyOffset++]);
            if (!desc.HS.pShaderBytecode) {
                continue;
            }
        }

        // Domain shader
        if (graphicsState->ds) {
            desc.DS = graphicsState->ds->GetInstrument(job.shaderInstrumentationKeys[keyOffset++]);
            if (!desc.DS.pShaderBytecode) {
                continue;
            }
        }

        // Geometry shader
        if (graphicsState->gs) {
            desc.GS = graphicsState->gs->GetInstrument(job.shaderInstrumentationKeys[keyOffset++]);
            if (!desc.GS.pShaderBytecode) {
                continue;
            }
        }

        // Pixel shader
        if (graphicsState->ps) {
            desc.PS = graphicsState->ps->GetInstrument(job.shaderInstrumentationKeys[keyOffset++]);
            if (!desc.PS.pShaderBytecode) {
                continue;
            }
        }

        // Destination pipeline
        ID3D12PipelineState* pipeline{nullptr};

        // Attempt to create pipeline
        HRESULT result = device->object->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline));
        if (FAILED(result)) {
            continue;
        }

        // Add pipeline
        state->AddInstrument(job.featureBitSet, pipeline);

        // Set the hot swapped object
        state->hotSwapObject.store(pipeline);
    }

    // Free bit sets
    for (uint32_t i = 0; i < batch.count; i++) {
        destroy(batch.jobs[i].shaderInstrumentationKeys, allocators);
    }

    // Free job
    destroy(batch.jobs, allocators);
}

void PipelineCompiler::CompileCompute(const PipelineJobBatch &batch) {
    // Populate all creation infos
    for (uint32_t i = 0; i < batch.count; i++) {
        PipelineJob &job = batch.jobs[i];
        PipelineState *state = job.state;

        // State
        auto computeState = static_cast<ComputePipelineState *>(state);

        // Copy the deep creation info
        //  This is safe, as the change is done on the copy itself, the original deep copy is untouched
        ASSERT(state->type == PipelineType::Compute, "Unexpected pipeline type");
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = computeState->deepCopy.desc;

        // Assign instrumented version
        desc.CS = computeState->cs->GetInstrument(job.shaderInstrumentationKeys[0]);
        if (!desc.CS.pShaderBytecode) {
            continue;
        }

        // Destination pipeline
        ID3D12PipelineState* pipeline{nullptr};

        // Attempt to create pipeline
        HRESULT result = device->object->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline));
        if (FAILED(result)) {
            continue;
        }

        // Add pipeline
        state->AddInstrument(job.featureBitSet, pipeline);

        // Set the hot swapped object
        state->hotSwapObject.store(pipeline);
    }

    // Free bit sets
    for (uint32_t i = 0; i < batch.count; i++) {
        destroy(batch.jobs[i].shaderInstrumentationKeys, allocators);
    }

    // Free job
    destroy(batch.jobs, allocators);
}
