#include <Backends/DX12/Compiler/PipelineCompiler.h>
#include <Backends/DX12/States/ShaderState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/SubObjectReader.h>

// Common
#include <Common/Dispatcher/Dispatcher.h>
#include <Common/Registry.h>

// TODO: Fix this pollution
#undef min
#undef max

PipelineCompiler::PipelineCompiler(DeviceState *device)
    : device(device),
      graphicsJobs(device->allocators.Tag(kAllocInstrumentation)),
      computeJobs(device->allocators.Tag(kAllocInstrumentation)) {

}

bool PipelineCompiler::Install() {
    dispatcher = registry->Get<Dispatcher>();
    if (!dispatcher) {
        return false;
    }

    // OK
    return true;
}

void PipelineCompiler::AddBatch(PipelineCompilerDiagnostic* diagnostic, PipelineJob *jobs, uint32_t count, DispatcherBucket *bucket) {
    // Segment the types
    for (uint32_t i = 0; i < count; i++) {
        switch (jobs[i].state->type) {
            default:
                ASSERT(false, "Invalid job type");
                break;
            case PipelineType::Graphics:
                graphicsJobs.push_back(jobs[i]);
                break;
            case PipelineType::Compute:
                computeJobs.push_back(jobs[i]);
                break;
        }
    }

    AddBatchOfType(diagnostic, graphicsJobs, PipelineType::Graphics, bucket);
    AddBatchOfType(diagnostic, computeJobs, PipelineType::Compute, bucket);

    graphicsJobs.clear();
    computeJobs.clear();
}

void PipelineCompiler::AddBatchOfType(PipelineCompilerDiagnostic* diagnostic, const Vector<PipelineJob> &jobs, PipelineType type, DispatcherBucket *bucket) {
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
        auto copy = new(allocators, kAllocInstrumentation) PipelineJob[count];
        std::memcpy(copy, &jobs[offset], sizeof(PipelineJob) * count);

        // Create the job data
        auto data = new(registry->GetAllocators(), kAllocInstrumentation) PipelineJobBatch{
            .diagnostic = diagnostic,
            .jobs = copy,
            .count = count
        };

        // Submit the job
        switch (type) {
            default:
                ASSERT(false, "Invalid job type");
                break;
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
    TrivialStackVector<uint8_t, 64'000> streamStack(allocators);

    // Device used for stream creates
    ID3D12Device2* streamDevice;

    // Query stream device
    if (FAILED(device->object->QueryInterface(__uuidof(ID3D12Device2), reinterpret_cast<void**>(&streamDevice)))) {
        streamDevice = nullptr;
    }

    // Populate all creation infos
    for (uint32_t i = 0; i < batch.count; i++) {
        PipelineJob &job = batch.jobs[i];
        PipelineState *state = job.state;

        // State
        ASSERT(state->type == PipelineType::Graphics, "Unexpected pipeline type");
        auto graphicsState = static_cast<GraphicsPipelineState *>(state);

        // Destination pipeline
        ID3D12PipelineState* pipeline{nullptr};

        // Stream based?
        if (!graphicsState->deepCopy.blob) {
            if (!streamDevice) {
                continue;
            }

            // Copy stream data
            streamStack.Resize(state->subObjectStreamBlob.size());
            std::memcpy(streamStack.Data(), state->subObjectStreamBlob.data(), state->subObjectStreamBlob.size());

            // Instrumentation offset
            uint32_t keyOffset{0};

            // Vertex shader
            if (graphicsState->vs) {
                D3D12_SHADER_BYTECODE overwrite = graphicsState->vs->GetInstrument(job.shaderInstrumentationKeys[keyOffset++]);
                if (!overwrite.pShaderBytecode) {
                    ++batch.diagnostic->failedJobs;
                    continue;
                }

                // Overwrite compute sub-object
                *reinterpret_cast<D3D12_SHADER_BYTECODE*>(streamStack.Data() + graphicsState->streamVSOffset) = overwrite;
            }

            // Hull shader
            if (graphicsState->hs) {
                D3D12_SHADER_BYTECODE overwrite = graphicsState->hs->GetInstrument(job.shaderInstrumentationKeys[keyOffset++]);
                if (!overwrite.pShaderBytecode) {
                    ++batch.diagnostic->failedJobs;
                    continue;
                }

                // Overwrite compute sub-object
                *reinterpret_cast<D3D12_SHADER_BYTECODE*>(streamStack.Data() + graphicsState->streamHSOffset) = overwrite;
            }

            // Domain shader
            if (graphicsState->ds) {
                D3D12_SHADER_BYTECODE overwrite = graphicsState->ds->GetInstrument(job.shaderInstrumentationKeys[keyOffset++]);
                if (!overwrite.pShaderBytecode) {
                    ++batch.diagnostic->failedJobs;
                    continue;
                }

                // Overwrite compute sub-object
                *reinterpret_cast<D3D12_SHADER_BYTECODE*>(streamStack.Data() + graphicsState->streamDSOffset) = overwrite;
            }

            // Geometry shader
            if (graphicsState->gs) {
                D3D12_SHADER_BYTECODE overwrite = graphicsState->gs->GetInstrument(job.shaderInstrumentationKeys[keyOffset++]);
                if (!overwrite.pShaderBytecode) {
                    ++batch.diagnostic->failedJobs;
                    continue;
                }

                // Overwrite compute sub-object
                *reinterpret_cast<D3D12_SHADER_BYTECODE*>(streamStack.Data() + graphicsState->streamGSOffset) = overwrite;
            }

            // Pixel shader
            if (graphicsState->ps) {
                D3D12_SHADER_BYTECODE overwrite = graphicsState->ps->GetInstrument(job.shaderInstrumentationKeys[keyOffset++]);
                if (!overwrite.pShaderBytecode) {
                    ++batch.diagnostic->failedJobs;
                    continue;
                }

                // Overwrite compute sub-object
                *reinterpret_cast<D3D12_SHADER_BYTECODE*>(streamStack.Data() + graphicsState->streamPSOffset) = overwrite;
            }

            // To stream description
            D3D12_PIPELINE_STATE_STREAM_DESC desc;
            desc.SizeInBytes = streamStack.Size();
            desc.pPipelineStateSubobjectStream = streamStack.Data();

            // Pass down callchain
            HRESULT hr = streamDevice->CreatePipelineState(&desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&pipeline));
            if (FAILED(hr)) {
                ++batch.diagnostic->failedJobs;
                continue;
            }
        } else {
            // Copy the deep creation info
            //  This is safe, as the change is done on the copy itself, the original deep copy is untouched
            D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = graphicsState->deepCopy.desc;

            // Instrumentation offset
            uint32_t keyOffset{0};

            // Vertex shader
            if (graphicsState->vs) {
                desc.VS = graphicsState->vs->GetInstrument(job.shaderInstrumentationKeys[keyOffset++]);
                if (!desc.VS.pShaderBytecode) {
                    ++batch.diagnostic->failedJobs;
                    continue;
                }
            }

            // Hull shader
            if (graphicsState->hs) {
                desc.HS = graphicsState->hs->GetInstrument(job.shaderInstrumentationKeys[keyOffset++]);
                if (!desc.HS.pShaderBytecode) {
                    ++batch.diagnostic->failedJobs;
                    continue;
                }
            }

            // Domain shader
            if (graphicsState->ds) {
                desc.DS = graphicsState->ds->GetInstrument(job.shaderInstrumentationKeys[keyOffset++]);
                if (!desc.DS.pShaderBytecode) {
                    ++batch.diagnostic->failedJobs;
                    continue;
                }
            }

            // Geometry shader
            if (graphicsState->gs) {
                desc.GS = graphicsState->gs->GetInstrument(job.shaderInstrumentationKeys[keyOffset++]);
                if (!desc.GS.pShaderBytecode) {
                    ++batch.diagnostic->failedJobs;
                    continue;
                }
            }

            // Pixel shader
            if (graphicsState->ps) {
                desc.PS = graphicsState->ps->GetInstrument(job.shaderInstrumentationKeys[keyOffset++]);
                if (!desc.PS.pShaderBytecode) {
                    ++batch.diagnostic->failedJobs;
                    continue;
                }
            }

            // Attempt to create pipeline
            HRESULT result = device->object->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline));
            if (FAILED(result)) {
                ++batch.diagnostic->failedJobs;
                continue;
            }
        }

        // Add pipeline
        state->AddInstrument(job.combinedHash, pipeline);
    }

    // Free bit sets
    for (uint32_t i = 0; i < batch.count; i++) {
        destroy(batch.jobs[i].shaderInstrumentationKeys, allocators);
    }

    // Free job
    destroy(batch.jobs, allocators);

    // Cleanup stream
    if (streamDevice) {
        streamDevice->Release();
    }
}

void PipelineCompiler::CompileCompute(const PipelineJobBatch &batch) {
    TrivialStackVector<uint8_t, 64'000> streamStack(allocators);

    // Device used for stream creates
    ID3D12Device2* streamDevice;

    // Query stream device
    if (FAILED(device->object->QueryInterface(__uuidof(ID3D12Device2), reinterpret_cast<void**>(&streamDevice)))) {
        streamDevice = nullptr;
    }

    // Populate all creation infos
    for (uint32_t i = 0; i < batch.count; i++) {
        PipelineJob &job = batch.jobs[i];
        PipelineState *state = job.state;

        // Destination pipeline
        ID3D12PipelineState* pipeline{nullptr};

        // State
        ASSERT(state->type == PipelineType::Compute, "Unexpected pipeline type");
        auto computeState = static_cast<ComputePipelineState *>(state);

        // Stream based?
        if (!computeState->deepCopy.blob) {
            if (!streamDevice) {
                ++batch.diagnostic->failedJobs;
                continue;
            }

            // Copy stream data
            streamStack.Resize(state->subObjectStreamBlob.size());
            std::memcpy(streamStack.Data(), state->subObjectStreamBlob.data(), state->subObjectStreamBlob.size());

            // Overwrite compute sub-object
            *reinterpret_cast<D3D12_SHADER_BYTECODE*>(streamStack.Data() + computeState->streamCSOffset) = computeState->cs->GetInstrument(job.shaderInstrumentationKeys[0]);

            // To stream description
            D3D12_PIPELINE_STATE_STREAM_DESC desc;
            desc.SizeInBytes = streamStack.Size();
            desc.pPipelineStateSubobjectStream = streamStack.Data();

            // Pass down callchain
            HRESULT hr = streamDevice->CreatePipelineState(&desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&pipeline));
            if (FAILED(hr)) {
                ++batch.diagnostic->failedJobs;
                continue;
            }
        } else {
            // Copy the deep creation info
            //  This is safe, as the change is done on the copy itself, the original deep copy is untouched
            D3D12_COMPUTE_PIPELINE_STATE_DESC desc = computeState->deepCopy.desc;

            // Assign instrumented version
            desc.CS = computeState->cs->GetInstrument(job.shaderInstrumentationKeys[0]);
            if (!desc.CS.pShaderBytecode) {
                ++batch.diagnostic->failedJobs;
                continue;
            }

            // Attempt to create pipeline
            HRESULT result = device->object->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline));
            if (FAILED(result)) {
                ++batch.diagnostic->failedJobs;
                continue;
            }
        }

        // Add pipeline
        state->AddInstrument(job.combinedHash, pipeline);
    }

    // Free bit sets
    for (uint32_t i = 0; i < batch.count; i++) {
        destroy(batch.jobs[i].shaderInstrumentationKeys, allocators);
    }

    // Free job
    destroy(batch.jobs, allocators);

    // Cleanup stream
    if (streamDevice) {
        streamDevice->Release();
    }
}
