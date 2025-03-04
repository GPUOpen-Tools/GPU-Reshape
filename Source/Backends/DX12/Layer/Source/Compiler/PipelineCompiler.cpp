// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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

#include <Backends/DX12/Compiler/PipelineCompiler.h>
#include <Backends/DX12/States/ShaderState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Compiler/DXBC/DXBCSigner.h>
#include <Backends/DX12/Compiler/DXIL/DXILSigner.h>
#include <Backends/DX12/Compiler/Diagnostic/DiagnosticType.h>
#include <Backends/DX12/StateObject.h>
#include <Backends/DX12/StateSubObjectWriter.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>
#include <Backends/DX12/States/RootSignatureState.h>
#include <Backends/DX12/States/StateObjectState.h>

// Backend
#include <Backend/Diagnostic/DiagnosticBucketScope.h>

// Common
#include <Common/Dispatcher/Dispatcher.h>
#include <Common/Registry.h>

// Std
#include <unordered_set>

// TODO: Fix this pollution
#undef min
#undef max

PipelineCompiler::PipelineCompiler(DeviceState *device)
    : device(device),
      graphicsJobs(device->allocators.Tag(kAllocInstrumentation)),
      computeJobs(device->allocators.Tag(kAllocInstrumentation)),
      stateObjectJobs(device->allocators.Tag(kAllocInstrumentation)) {

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
    std::lock_guard guard(mutex);

    // Increment jobs
    diagnostic->totalJobs += count;
    
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
            case PipelineType::StateObject:
                stateObjectJobs.push_back(jobs[i]);
                break;
        }
    }

    // Submit jobs
    AddBatchOfType(diagnostic, graphicsJobs, PipelineType::Graphics, bucket);
    AddBatchOfType(diagnostic, computeJobs, PipelineType::Compute, bucket);
    AddBatchOfType(diagnostic, stateObjectJobs, PipelineType::StateObject, bucket);

    // Cleanup
    graphicsJobs.clear();
    computeJobs.clear();
    stateObjectJobs.clear();
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
            case PipelineType::StateObject:
                dispatcher->Add(BindDelegate(this, PipelineCompiler::WorkerStateObject), data, bucket);
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

void PipelineCompiler::WorkerStateObject(void *data) {
    auto *job = static_cast<PipelineJobBatch *>(data);
    CompileStateObject(*job);
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

        // Diagnostic scope
        DiagnosticBucketScope scope(batch.diagnostic->messages, job.state->uid);

        // State
        ASSERT(state->type == PipelineType::Graphics, "Unexpected pipeline type");
        auto graphicsState = static_cast<GraphicsPipelineState *>(state);

        // Destination pipeline
        ID3D12PipelineState* pipeline{nullptr};

        // Stream based?
        if (!graphicsState->deepCopy.valid) {
            if (!streamDevice) {
                scope.Add(DiagnosticType::PipelineInternalError);
                ++batch.diagnostic->failedJobs;
                continue;
            }

            // Get deep copy
            D3D12_PIPELINE_STATE_STREAM_DESC deepCopyDesc = state->subObjectWriter.GetDesc();

            // Copy stream data
            streamStack.Resize(deepCopyDesc.SizeInBytes);
            std::memcpy(streamStack.Data(), deepCopyDesc.pPipelineStateSubobjectStream, deepCopyDesc.SizeInBytes);

            // Instrumentation offset
            uint32_t keyOffset{0};

            // Vertex shader
            if (graphicsState->vs) {
                D3D12_SHADER_BYTECODE overwrite = graphicsState->vs->GetInstrument(job.shaderInstrumentationKeys[keyOffset++].shaderKey);
                if (!overwrite.pShaderBytecode) {
                    scope.Add(DiagnosticType::PipelineMissingShaderKey);
                    ++batch.diagnostic->failedJobs;
                    continue;
                }

                // Overwrite compute sub-object
                *reinterpret_cast<D3D12_SHADER_BYTECODE*>(streamStack.Data() + graphicsState->streamVSOffset) = overwrite;
            }

            // Hull shader
            if (graphicsState->hs) {
                D3D12_SHADER_BYTECODE overwrite = graphicsState->hs->GetInstrument(job.shaderInstrumentationKeys[keyOffset++].shaderKey);
                if (!overwrite.pShaderBytecode) {
                    scope.Add(DiagnosticType::PipelineMissingShaderKey);
                    ++batch.diagnostic->failedJobs;
                    continue;
                }

                // Overwrite compute sub-object
                *reinterpret_cast<D3D12_SHADER_BYTECODE*>(streamStack.Data() + graphicsState->streamHSOffset) = overwrite;
            }

            // Domain shader
            if (graphicsState->ds) {
                D3D12_SHADER_BYTECODE overwrite = graphicsState->ds->GetInstrument(job.shaderInstrumentationKeys[keyOffset++].shaderKey);
                if (!overwrite.pShaderBytecode) {
                    scope.Add(DiagnosticType::PipelineMissingShaderKey);
                    ++batch.diagnostic->failedJobs;
                    continue;
                }

                // Overwrite compute sub-object
                *reinterpret_cast<D3D12_SHADER_BYTECODE*>(streamStack.Data() + graphicsState->streamDSOffset) = overwrite;
            }

            // Geometry shader
            if (graphicsState->gs) {
                D3D12_SHADER_BYTECODE overwrite = graphicsState->gs->GetInstrument(job.shaderInstrumentationKeys[keyOffset++].shaderKey);
                if (!overwrite.pShaderBytecode) {
                    scope.Add(DiagnosticType::PipelineMissingShaderKey);
                    ++batch.diagnostic->failedJobs;
                    continue;
                }

                // Overwrite compute sub-object
                *reinterpret_cast<D3D12_SHADER_BYTECODE*>(streamStack.Data() + graphicsState->streamGSOffset) = overwrite;
            }

            // Pixel shader
            if (graphicsState->ps) {
                D3D12_SHADER_BYTECODE overwrite = graphicsState->ps->GetInstrument(job.shaderInstrumentationKeys[keyOffset++].shaderKey);
                if (!overwrite.pShaderBytecode) {
                    scope.Add(DiagnosticType::PipelineMissingShaderKey);
                    ++batch.diagnostic->failedJobs;
                    continue;
                }

                // Overwrite compute sub-object
                *reinterpret_cast<D3D12_SHADER_BYTECODE*>(streamStack.Data() + graphicsState->streamPSOffset) = overwrite;
            }

            // Amplification shader
            if (graphicsState->as) {
                D3D12_SHADER_BYTECODE overwrite = graphicsState->as->GetInstrument(job.shaderInstrumentationKeys[keyOffset++].shaderKey);
                if (!overwrite.pShaderBytecode) {
                    scope.Add(DiagnosticType::PipelineMissingShaderKey);
                    ++batch.diagnostic->failedJobs;
                    continue;
                }

                // Overwrite compute sub-object
                *reinterpret_cast<D3D12_SHADER_BYTECODE *>(streamStack.Data() + graphicsState->streamASOffset) = overwrite;
            }

            // Mesh shader
            if (graphicsState->ms) {
                D3D12_SHADER_BYTECODE overwrite = graphicsState->ms->GetInstrument(job.shaderInstrumentationKeys[keyOffset++].shaderKey);
                if (!overwrite.pShaderBytecode) {
                    scope.Add(DiagnosticType::PipelineMissingShaderKey);
                    ++batch.diagnostic->failedJobs;
                    continue;
                }

                // Overwrite compute sub-object
                *reinterpret_cast<D3D12_SHADER_BYTECODE *>(streamStack.Data() + graphicsState->streamMSOffset) = overwrite;
            }

            // To stream description
            D3D12_PIPELINE_STATE_STREAM_DESC desc;
            desc.SizeInBytes = streamStack.Size();
            desc.pPipelineStateSubobjectStream = streamStack.Data();

            // Pass down callchain
            HRESULT hr = streamDevice->CreatePipelineState(&desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&pipeline));
            if (FAILED(hr)) {
                scope.Add(DiagnosticType::PipelineCreationFailed);
                ++batch.diagnostic->failedJobs;
                continue;
            }
        } else {
            // Copy the deep creation info
            //  This is safe, as the change is done on the copy itself, the original deep copy is untouched
            D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = graphicsState->deepCopy.desc;

            // Ignore cache
            desc.CachedPSO = {};

            // Instrumentation offset
            uint32_t keyOffset{0};

            // Vertex shader
            if (graphicsState->vs) {
                desc.VS = graphicsState->vs->GetInstrument(job.shaderInstrumentationKeys[keyOffset++].shaderKey);
                if (!desc.VS.pShaderBytecode) {
                    scope.Add(DiagnosticType::PipelineMissingShaderKey);
                    ++batch.diagnostic->failedJobs;
                    continue;
                }
            }

            // Hull shader
            if (graphicsState->hs) {
                desc.HS = graphicsState->hs->GetInstrument(job.shaderInstrumentationKeys[keyOffset++].shaderKey);
                if (!desc.HS.pShaderBytecode) {
                    scope.Add(DiagnosticType::PipelineMissingShaderKey);
                    ++batch.diagnostic->failedJobs;
                    continue;
                }
            }

            // Domain shader
            if (graphicsState->ds) {
                desc.DS = graphicsState->ds->GetInstrument(job.shaderInstrumentationKeys[keyOffset++].shaderKey);
                if (!desc.DS.pShaderBytecode) {
                    scope.Add(DiagnosticType::PipelineMissingShaderKey);
                    ++batch.diagnostic->failedJobs;
                    continue;
                }
            }

            // Geometry shader
            if (graphicsState->gs) {
                desc.GS = graphicsState->gs->GetInstrument(job.shaderInstrumentationKeys[keyOffset++].shaderKey);
                if (!desc.GS.pShaderBytecode) {
                    scope.Add(DiagnosticType::PipelineMissingShaderKey);
                    ++batch.diagnostic->failedJobs;
                    continue;
                }
            }

            // Pixel shader
            if (graphicsState->ps) {
                desc.PS = graphicsState->ps->GetInstrument(job.shaderInstrumentationKeys[keyOffset++].shaderKey);
                if (!desc.PS.pShaderBytecode) {
                    scope.Add(DiagnosticType::PipelineMissingShaderKey);
                    ++batch.diagnostic->failedJobs;
                    continue;
                }
            }

            // AS and MS not supported yet
            ASSERT(!graphicsState->as, "Not supported");
            ASSERT(!graphicsState->ms, "Not supported");

            // Attempt to create pipeline
            HRESULT result = device->object->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline));
            if (FAILED(result)) {
                scope.Add(DiagnosticType::PipelineCreationFailed);
                ++batch.diagnostic->failedJobs;
                continue;
            }
        }

        // Add pipeline
        state->AddInstrument(job.combinedHash, pipeline);

        // Mark as passed
        ++batch.diagnostic->passedJobs;
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

        // Diagnostic scope
        DiagnosticBucketScope scope(batch.diagnostic->messages, job.state->uid);

        // Destination pipeline
        ID3D12PipelineState* pipeline{nullptr};

        // State
        ASSERT(state->type == PipelineType::Compute, "Unexpected pipeline type");
        auto computeState = static_cast<ComputePipelineState *>(state);

        // Stream based?
        if (!computeState->deepCopy.valid) {
            if (!streamDevice) {
                scope.Add(DiagnosticType::PipelineInternalError);
                ++batch.diagnostic->failedJobs;
                continue;
            }

            // Get deep copy
            D3D12_PIPELINE_STATE_STREAM_DESC deepCopyDesc = state->subObjectWriter.GetDesc();

            // Copy stream data
            streamStack.Resize(deepCopyDesc.SizeInBytes);
            std::memcpy(streamStack.Data(), deepCopyDesc.pPipelineStateSubobjectStream, deepCopyDesc.SizeInBytes);

            // Overwrite compute sub-object
            *reinterpret_cast<D3D12_SHADER_BYTECODE*>(streamStack.Data() + computeState->streamCSOffset) = computeState->cs->GetInstrument(job.shaderInstrumentationKeys[0].shaderKey);

            // To stream description
            D3D12_PIPELINE_STATE_STREAM_DESC desc;
            desc.SizeInBytes = streamStack.Size();
            desc.pPipelineStateSubobjectStream = streamStack.Data();

            // Pass down callchain
            HRESULT hr = streamDevice->CreatePipelineState(&desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&pipeline));
            if (FAILED(hr)) {
                scope.Add(DiagnosticType::PipelineCreationFailed);
                ++batch.diagnostic->failedJobs;
                continue;
            }
        } else {
            // Copy the deep creation info
            //  This is safe, as the change is done on the copy itself, the original deep copy is untouched
            D3D12_COMPUTE_PIPELINE_STATE_DESC desc = computeState->deepCopy.desc;

            // Ignore cache
            desc.CachedPSO = {};

            // Assign instrumented version
            desc.CS = computeState->cs->GetInstrument(job.shaderInstrumentationKeys[0].shaderKey);
            if (!desc.CS.pShaderBytecode) {
                scope.Add(DiagnosticType::PipelineMissingShaderKey);
                ++batch.diagnostic->failedJobs;
                continue;
            }

            // Attempt to create pipeline
            HRESULT result = device->object->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline));
            if (FAILED(result)) {
                scope.Add(DiagnosticType::PipelineCreationFailed);
                ++batch.diagnostic->failedJobs;
                continue;
            }
        }

        // Add pipeline
        state->AddInstrument(job.combinedHash, pipeline);

        // Mark as passed
        ++batch.diagnostic->passedJobs;
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

void PipelineCompiler::CompileStateObject(const PipelineJobBatch &batch) {
    // Device used for stream creates
    ID3D12Device7* streamDevice;

    // Query stream device
    if (FAILED(device->object->QueryInterface(__uuidof(ID3D12Device7), reinterpret_cast<void**>(&streamDevice)))) {
        streamDevice = nullptr;
    }

    // Populate all creation infos
    for (uint32_t i = 0; i < batch.count; i++) {
        PipelineJob &job = batch.jobs[i];
        PipelineState *state = job.state;

        // Diagnostic scope
        DiagnosticBucketScope scope(batch.diagnostic->messages, job.state->uid);

        // Destination pipeline
        ID3D12StateObject* stateObject{nullptr};

        // State
        ASSERT(state->type == PipelineType::StateObject, "Unexpected pipeline type");
        auto stateObjectState = static_cast<StateObjectState *>(state);

        // New sub-object writer
        StateSubObjectWriter writer(device->allocators);

        // All the exports that we've replaced, used for filtering
        std::unordered_set<std::wstring> replacedExports;

        // Local cache
        std::vector<D3D12_EXPORT_DESC> localExports;
        std::vector<LPCWSTR>           localNames;

        // All associations that have yet to be pushed
        std::vector<const StateShaderSubObject*> pendingAssociations;

        // Source, unwrapped, writer for inheriting configurations
        auto originalDesc = stateObjectState->writer.GetDesc(stateObjectState->stateObjectType);

        // TODO[rt]: Lookup time might not be ok, consider having a one-to-many lookup
        for (const StateShaderSubObject &subObject: stateObjectState->shaderSubObjects) {
            // Designated key, if a key isn't found, append the state object anyway which fetches the default sub-object
            ShaderInstrumentationKey localKey{};
            
            // Find the matching key
            for (uint32_t keyIndex = 0; keyIndex < job.keyCount; keyIndex++) {
                const PipelineJobKey& key = job.shaderInstrumentationKeys[keyIndex];
                
                // Not the replaced shader? Skip
                // May not be instrumented, just keep the sub-object as is
                if (subObject.shader != key.shader || !key.shaderKey.featureBitSet) {
                    continue;
                }

                // Append the local mappings
                localKey = key.shaderKey;

                // Combine hashes
                for (const StateShaderSubObjectExport &_export: subObject.functionExports) {
                    if (_export.localSignature) {
                        CombineHash(localKey.combinedHash, _export.localSignature->physicalMapping->signatureHash);
                    }
                }

                // Found the key, stop
                break;
            }
            
            // Get the instrumented blob
            D3D12_SHADER_BYTECODE byteCode = subObject.shader->GetInstrument(localKey);
            if (!byteCode.pShaderBytecode) {
                scope.Add(DiagnosticType::PipelineMissingShaderKey);
                ++batch.diagnostic->failedJobs;
                continue;
            }

            // We implicitly instrument all exports, so pull them all in
            for (const StateShaderSubObjectExport &_export: subObject.functionExports) {
                localExports.push_back(D3D12_EXPORT_DESC{
                    .Name = _export.name.c_str(),
                    .ExportToRename = writer.EmbedAnsi(_export.dxbc.unmangledName)
                });

                // Do not inherit this export
                replacedExports.insert(_export.name);
            }

            // Add instrumented library
            writer.Add(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, D3D12_DXIL_LIBRARY_DESC{
                .DXILLibrary = byteCode,
                .NumExports = static_cast<UINT>(localExports.size()),
                .pExports = static_cast<const D3D12_EXPORT_DESC *>(writer.Embed(localExports.data(), static_cast<uint32_t>(localExports.size() * sizeof(D3D12_EXPORT_DESC))))
            });

            // Associate later
            pendingAssociations.push_back(&subObject);

            // Cleanup
            localExports.clear();
        }

        // Always re-emit hit groups
        // We can't inherit it from the existing collection, as those exports are technically re-exported
        for (const D3D12_HIT_GROUP_DESC& hitGroup : stateObjectState->hitGroupSubobjects) {
            writer.Add(D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, hitGroup);
        } 

        // Append configurations
        for (uint32_t subObjectIndex = 0; subObjectIndex < originalDesc.NumSubobjects; subObjectIndex++) {
            const D3D12_STATE_SUBOBJECT& subObject = originalDesc.pSubobjects[subObjectIndex];

            // Either directly handled or inherited
            if (subObject.Type == D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION ||
                subObject.Type == D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION ||
                subObject.Type == D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION ||
                subObject.Type == D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP ||
                subObject.Type == D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY ||
                subObject.Type == D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE) {
                continue;
            }

            // Add config subobject
            writer.DeepAdd(subObject.Type, subObject.pDesc);
        }

        // Write associations
        for (const StateShaderSubObject *subObject: pendingAssociations) {
            // Associate all export specific states
            for (const StateShaderSubObjectExport& _export : subObject->functionExports) {
                // Rewrite local root signature, if any
                if (_export.localSignature) {
                    writer.Add(D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, D3D12_LOCAL_ROOT_SIGNATURE { .pLocalRootSignature = _export.localSignature->object });
                    writer.SubObjectAssociation(_export.name.c_str(), static_cast<uint32_t>(writer.SubObjectCount()) - 1);
                }

                // Rewrite all inline associations
                for (const StateSubObjectAssociation& association : _export.associations) {
                    writer.Add(association.type, static_cast<const void*>(association.data.Data()));
                    writer.SubObjectAssociation(_export.name.c_str(), static_cast<uint32_t>(writer.SubObjectCount()) - 1);
                }
            }
        }

        // Create description
        D3D12_STATE_OBJECT_DESC desc = writer.GetDesc(stateObjectState->stateObjectType);

        // Try to create
        HRESULT hr = streamDevice->CreateStateObject(&desc, __uuidof(ID3D12StateObject), reinterpret_cast<void**>(&stateObject));
        if (FAILED(hr)) {
            scope.Add(DiagnosticType::PipelineCreationFailed);
            ++batch.diagnostic->failedJobs;
            continue;
        }

        // Compile and add new patch
        stateObjectState->AddPatch(job.combinedHash, CreateStateObjectShaderIdentifierPatch(stateObjectState, stateObject));

        // Add state
        state->AddInstrument(job.combinedHash, stateObject);

        // Mark as passed
        ++batch.diagnostic->passedJobs;
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
