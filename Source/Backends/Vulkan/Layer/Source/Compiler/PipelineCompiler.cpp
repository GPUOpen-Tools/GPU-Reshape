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

#include <Backends/Vulkan/Compiler/PipelineCompiler.h>
#include <Backends/Vulkan/States/ShaderModuleState.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Compiler/Diagnostic/DiagnosticType.h>

// Backend
#include <Backend/Diagnostic/DiagnosticBucketScope.h>

// Common
#include <Common/Dispatcher/Dispatcher.h>
#include <Common/Registry.h>

// The maximum number per batch
//  Each compilation job is of varying time, so split up the batches across workers to ensure that they're spread out
//  to whatever workers first take them
constexpr uint32_t kMaxBatchSize = 64;

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

void PipelineCompiler::AddBatch(DeviceDispatchTable *table, PipelineCompilerDiagnostic* diagnostic, PipelineJob *jobs, uint32_t count, DispatcherBucket *bucket) {
    std::lock_guard guard(mutex);

    // Increment jobs
    diagnostic->totalJobs += count;
    
    // Segment the types
    for (uint32_t i = 0; i < count; i++) {
        switch (jobs[i].state->type) {
            default:
                ASSERT(false, "Invalid pipeline type");
                break;
            case PipelineType::Graphics:
                graphicsJobs.push_back(jobs[i]);
                break;
            case PipelineType::Compute:
                computeJobs.push_back(jobs[i]);
                break;
        }
    }

    // Submit jobs
    AddBatchOfType(table, diagnostic, graphicsJobs, PipelineType::Graphics, bucket);
    AddBatchOfType(table, diagnostic, computeJobs, PipelineType::Compute, bucket);

    // Cleanup
    graphicsJobs.clear();
    computeJobs.clear();
}

void PipelineCompiler::AddBatchOfType(DeviceDispatchTable *table, PipelineCompilerDiagnostic* diagnostic, const std::vector<PipelineJob> &jobs, PipelineType type, DispatcherBucket *bucket) {
    if (jobs.empty()) {
        return;
    }

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
            .table = table,
            .diagnostic = diagnostic,
            .jobs = copy,
            .count = count
        };

        // Submit the job
        switch (type) {
            default:
                ASSERT(false, "Invalid pipeline type");
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

bool PipelineCompiler::SetShaderModuleObject(VkPipelineShaderStageCreateInfo &createInfo, ShaderModuleState *state, const ShaderModuleInstrumentationKey &key) {
    // If there's an instrumentation key, a module must have been created
    if (key) {
        createInfo.pNext = nullptr;
        createInfo.module = state->GetInstrument(key);
        return createInfo.module != nullptr;
    }

    // If there's a default object, use that
    if (state->object) {
        createInfo.pNext = nullptr;
        createInfo.module = state->object;
        return true;
    }

    // Otherwise, try to check for an inlined creation structure
    if (FindStructureTypeSafe<VkShaderModuleCreateInfo>(&createInfo, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO)) {
        createInfo.module = nullptr;
        return true;
    }

    // Unknown state
    return false;
}

void PipelineCompiler::CompileGraphics(const PipelineJobBatch &batch) {
    // Allocate creation info
    TrivialStackVector<VkGraphicsPipelineCreateInfo, kMaxBatchSize> createInfos;
    TrivialStackVector<uint32_t,                     kMaxBatchSize> jobIndices;

    // Optional, deep copies
    VkGraphicsPipelineCreateInfoDeepCopy deepCopies[kMaxBatchSize];

    // Intermediate counts
    uint32_t stageCount = 0;
    uint32_t libraryStateCount = 0;
    
    // Get the number of stages 
    for (uint32_t i = 0; i < batch.count; i++) {
        PipelineJob &job = batch.jobs[i];
        stageCount += static_cast<uint32_t>(job.state->ownedShaderModules.size());
        
        if (job.state->pipelineLibraries.size()) {
            libraryStateCount += static_cast<uint32_t>(job.state->pipelineLibraries.size());
        }
    }

    // Allocate intermediate data
    auto stageInfos         = ALLOCA_ARRAY(VkPipelineShaderStageCreateInfo, stageCount);
    auto libraryStates      = ALLOCA_ARRAY(VkPipeline                     , libraryStateCount);

    // Populate all creation infos
    for (uint32_t i = 0; i < batch.count; i++) {
        PipelineJob &job = batch.jobs[i];
        PipelineState *state = job.state;

        // Current creation info
        VkGraphicsPipelineCreateInfo createInfo;

        // Diagnostic scope
        DiagnosticBucketScope scope(batch.diagnostic->messages, job.state->uid);

        // Any extension mutation requires a deep copy
        bool requiresMutableExtensions = false;

        // Pipeline libraries are fed through extensions
        if (job.state->pipelineLibraries.size()) {
            requiresMutableExtensions = true;
        }

        // Copy the deep creation info
        //  This is safe, as the change is done on the copy itself, the original deep copy is untouched
        ASSERT(state->type == PipelineType::Graphics, "Unexpected pipeline type");
        if (requiresMutableExtensions) {
            VkGraphicsPipelineCreateInfoDeepCopy& deepCopy = deepCopies[i];
            deepCopy.DeepCopy(allocators, static_cast<GraphicsPipelineState *>(state)->createInfoDeepCopy.createInfo);
            createInfo = deepCopy.createInfo;
        } else {
            createInfo = static_cast<GraphicsPipelineState *>(state)->createInfoDeepCopy.createInfo;
        }

        // Exclude irrelevant flags
        createInfo.flags &= ~(
            VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT |
            VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT
        );

        // All keys must be present for pipeline compilation
        bool anyMissingKeys = false;

        // Copy all libraries
        if (job.state->pipelineLibraries.size()) {
            auto* libraryCreateInfo = FindStructureTypeMutableUnsafe<VkPipelineLibraryCreateInfoKHR, VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR>(&createInfo);
            libraryCreateInfo->pLibraries = libraryStates;

            // Fill pipelines
            for (uint32_t libraryIndex = 0; libraryIndex < libraryCreateInfo->libraryCount; libraryIndex++) {
                PipelineState *libraryState = state->pipelineLibraries[libraryIndex];
                libraryStates[libraryIndex] = libraryState->GetInstrument(job.pipelineLibraryInstrumentationKeys[libraryIndex]);
                
                // Validate
                if (!libraryStates[libraryIndex]) {
                    scope.Add(DiagnosticType::PipelineMissingShaderKey);
                    anyMissingKeys = true;
                }
            }

            // Next!
            libraryStates += job.state->pipelineLibraries.size();
        }

        // Set new instrumented stages
        for (uint32_t shaderIndex = 0; shaderIndex < state->ownedShaderModules.size(); shaderIndex++) {
            ShaderModuleState *shaderState = state->ownedShaderModules[shaderIndex];
            std::memcpy(&stageInfos[shaderIndex], &createInfo.pStages[shaderIndex], sizeof(VkPipelineShaderStageCreateInfo));

            // Try to set shader object
            if (!SetShaderModuleObject(stageInfos[shaderIndex], shaderState, job.shaderModuleInstrumentationKeys[shaderIndex])) {
                scope.Add(DiagnosticType::PipelineMissingShaderKey);
                anyMissingKeys = true;
            }
        }

        // If any keys were missing, the entire pipeline has failed
        if (anyMissingKeys) {
            continue;
        }

        // Set new stage info
        createInfo.pStages = stageInfos;
        stageInfos += state->ownedShaderModules.size();

        // OK
        createInfos.Add(createInfo);
        jobIndices.Add(i);
    }

    // Created pipelines
    auto pipelines = ALLOCA_ARRAY(VkPipeline, batch.count);

    // Batched?
#if PIPELINE_COMPILER_NO_BATCH
    for (uint32_t i = 0; i <  static_cast<uint32_t>(createInfos.Size()); i++) {
        VkResult result = batch.table->next_vkCreateGraphicsPipelines(batch.table->object, nullptr, 1u, &createInfos[i], nullptr, &pipelines[i]);
        if (result != VK_SUCCESS) {
            ++batch.diagnostic->failedJobs;
            return;
        }

        // Mark as passed
        ++batch.diagnostic->passedJobs;
    }
#else
    // TODO: Pipeline cache?
    VkResult result = batch.table->next_vkCreateGraphicsPipelines(batch.table->object, nullptr, static_cast<uint32_t>(createInfos.Size()), createInfos.Data(), nullptr, pipelines);
    if (result != VK_SUCCESS) {
        // Add diagnostics for all failed pipelines
        for (uint32_t i = 0; i < batch.count; i++) {
            batch.diagnostic->messages->Add(DiagnosticType::PipelineCreationFailed, batch.jobs[i].state->uid);
        }
        
        batch.diagnostic->failedJobs += batch.count;
        return;
    }

    // Mark as passed
    batch.diagnostic->passedJobs += batch.count;
#endif

    // Set final objects
    for (uint32_t i = 0; i < static_cast<uint32_t>(createInfos.Size()); i++) {
        PipelineJob &job = batch.jobs[jobIndices[i]];
        PipelineState *state = job.state;

        // Set the instrument for the given hash
        state->AddInstrument(job.combinedHash, pipelines[i]);
    }

    // Free keys
    for (uint32_t i = 0; i < batch.count; i++) {
        destroy(batch.jobs[i].shaderModuleInstrumentationKeys, allocators);
        destroy(batch.jobs[i].pipelineLibraryInstrumentationKeys, allocators);
    }

    // Free job
    destroy(batch.jobs, allocators);
}

void PipelineCompiler::CompileCompute(const PipelineJobBatch &batch) {
    // Allocate creation info
    TrivialStackVector<VkComputePipelineCreateInfo, kMaxBatchSize> createInfos;
    TrivialStackVector<uint32_t,                    kMaxBatchSize> jobIndices;

    // Optional, all deep copies
    VkComputePipelineCreateInfoDeepCopy deepCopies[kMaxBatchSize];

    // Intermediate counts
    uint32_t libraryStateCount = 0;
    
    // Get the number of stages 
    for (uint32_t i = 0; i < batch.count; i++) {
        PipelineJob &job = batch.jobs[i];
        
        if (job.state->pipelineLibraries.size()) {
            libraryStateCount += static_cast<uint32_t>(job.state->pipelineLibraries.size());
        }
    }
    
    // Allocate intermediate data
    auto libraryStates = ALLOCA_ARRAY(VkPipeline, libraryStateCount);
    
    // Populate all creation infos
    for (uint32_t i = 0; i < batch.count; i++) {
        PipelineJob &job = batch.jobs[i];
        PipelineState *state = job.state;

        // Creation info
        VkComputePipelineCreateInfo createInfo;
        
        // Diagnostic scope
        DiagnosticBucketScope scope(batch.diagnostic->messages, job.state->uid);

        // Any extension mutation requires a deep copy
        bool requiresMutableExtensions = false;
        
        // Pipeline libraries are fed through extensions
        if (job.state->pipelineLibraries.size()) {
            requiresMutableExtensions = true;
        }

        // Copy the deep creation info
        //  This is safe, as the change is done on the copy itself, the original deep copy is untouched
        ASSERT(state->type == PipelineType::Compute, "Unexpected pipeline type");
        if (requiresMutableExtensions) {
            VkComputePipelineCreateInfoDeepCopy& deepCopy = deepCopies[i];
            deepCopy.DeepCopy(allocators, static_cast<ComputePipelineState *>(state)->createInfoDeepCopy.createInfo);
            createInfo = deepCopy.createInfo;
        } else {
            createInfo = static_cast<ComputePipelineState *>(state)->createInfoDeepCopy.createInfo;
        }

        // Exclude irrelevant flags
        createInfo.flags &= ~(
            VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT |
            VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT
        );

        // All keys must be present for pipeline compilation
        bool anyMissingKeys = false;

        // Copy all libraries
        if (job.state->pipelineLibraries.size()) {
            auto* libraryCreateInfo = FindStructureTypeMutableUnsafe<VkPipelineLibraryCreateInfoKHR, VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR>(&createInfo);
            libraryCreateInfo->pLibraries = libraryStates;

            // Fill pipelines
            for (uint32_t libraryIndex = 0; libraryIndex < libraryCreateInfo->libraryCount; libraryIndex++) {
                PipelineState *libraryState = state->pipelineLibraries[libraryIndex];
                libraryStates[libraryIndex] = libraryState->GetInstrument(job.pipelineLibraryInstrumentationKeys[libraryIndex]);
                
                // Validate
                if (!libraryStates[libraryIndex]) {
                    scope.Add(DiagnosticType::PipelineMissingShaderKey);
                    anyMissingKeys = true;
                }
            }

            // Next!
            libraryStates += job.state->pipelineLibraries.size();
        }

        // Get the shader
        ASSERT(state->ownedShaderModules.size() == 1, "Compute pipeline expected one shader module");
        ShaderModuleState *shaderState = state->ownedShaderModules[0];

        // Assign instrumented version
        if (!SetShaderModuleObject(createInfo.stage, shaderState, job.shaderModuleInstrumentationKeys[0])) {
            scope.Add(DiagnosticType::PipelineMissingShaderKey);
        }

        // Validate
        if (!createInfo.stage.module) {
            scope.Add(DiagnosticType::PipelineMissingShaderKey);
            anyMissingKeys = true;
        }

        // If any keys were missing, the entire pipeline has failed
        if (anyMissingKeys) {
            continue;
        }

        // OK
        createInfos.Add(createInfo);
        jobIndices.Add(i);
    }

    // Created pipelines
    auto pipelines = ALLOCA_ARRAY(VkPipeline, batch.count);

    // TODO: Pipeline cache?
    VkResult result = batch.table->next_vkCreateComputePipelines(batch.table->object, nullptr, static_cast<uint32_t>(createInfos.Size()), createInfos.Data(), nullptr, pipelines);
    if (result != VK_SUCCESS) {
        // Add diagnostics for all failed pipelines
        for (uint32_t i = 0; i < batch.count; i++) {
            batch.diagnostic->messages->Add(DiagnosticType::PipelineCreationFailed, batch.jobs[i].state->uid);
        }
        
        batch.diagnostic->failedJobs += batch.count;
        return;
    }

    // Mark as passed
    batch.diagnostic->passedJobs += batch.count;

    // Set final objects
    for (uint32_t i = 0; i < static_cast<uint32_t>(createInfos.Size()); i++) {
        PipelineJob &job = batch.jobs[jobIndices[i]];
        PipelineState *state = job.state;

        // Set the instrument for the given hash
        state->AddInstrument(job.combinedHash, pipelines[i]);
    }

    // Free bit sets
    for (uint32_t i = 0; i < batch.count; i++) {
        destroy(batch.jobs[i].shaderModuleInstrumentationKeys, allocators);
        destroy(batch.jobs[i].pipelineLibraryInstrumentationKeys, allocators);
    }

    // Free job
    destroy(batch.jobs, allocators);
}
