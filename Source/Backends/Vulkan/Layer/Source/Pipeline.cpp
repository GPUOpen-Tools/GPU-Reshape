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

#include <Backends/Vulkan/States/PipelineState.h>
#include <Backends/Vulkan/States/ShaderModuleState.h>
#include <Backends/Vulkan/States/PipelineLayoutState.h>
#include <Backends/Vulkan/States/RenderPassState.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Controllers/InstrumentationController.h>

static ShaderModuleState* GetPipelineStageShaderModule(DeviceDispatchTable* table, const VkPipelineShaderStageCreateInfo& createInfo) {
    // If there's a stage, just return it
    if (createInfo.module) {
        return table->states_shaderModule.Get(createInfo.module);
    }

    // Pipeline stages may supply the module info by extension
    // Create a dummy internal state without an actual module handle
    if (auto* moduleCreateInfo = FindStructureTypeSafe<VkShaderModuleCreateInfo>(&createInfo, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO)) {
        // Allocate state, reference added externally
        auto state = new (table->allocators) ShaderModuleState;
        state->table = table;
        state->object = nullptr;
        state->createInfoDeepCopy.DeepCopy(table->allocators, *moduleCreateInfo);

        // Keep track of it
        table->states_shaderModule.Add(nullptr, state);
        return state;
    }

    ASSERT(false, "Unsupported path");
    return nullptr;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Delay writeout
    auto pipelines = ALLOCA_ARRAY(VkPipeline, createInfoCount);

    // Pass down callchain
    VkResult result = table->next_vkCreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pipelines);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Allocate states
    for (uint32_t i = 0; i < createInfoCount; i++) {
        auto state = new (table->allocators) GraphicsPipelineState;
        state->type = PipelineType::Graphics;
        state->table = table;
        state->object = pipelines[i];
        state->isLibrary = (pCreateInfos[i].flags & VK_PIPELINE_CREATE_LIBRARY_BIT_KHR);
        state->createInfoDeepCopy.DeepCopy(table->allocators, pCreateInfos[i]);

        // External user
        state->AddUser();

        // Add a reference to the layout
        if (pCreateInfos[i].layout) {
            state->layout = table->states_pipelineLayout.Get(pCreateInfos[i].layout);
            state->layout->AddUser();
        } else {
            ASSERT(state->isLibrary, "Expected pipeline layout on non-library pipelines");
        }

        // Add reference to the render pass
        if (pCreateInfos[i].renderPass) {
            state->renderPass = table->states_renderPass.Get(pCreateInfos[i].renderPass);
            state->renderPass->AddUser();
        }

        // Collect all shader modules
        for (uint32_t stageIndex = 0; stageIndex < state->createInfoDeepCopy.createInfo.stageCount; stageIndex++) {
            const VkPipelineShaderStageCreateInfo& stageInfo = state->createInfoDeepCopy.createInfo.pStages[stageIndex];

            // Get the proxied state
            ShaderModuleState* shaderModuleState = GetPipelineStageShaderModule(table, stageInfo);

            // Add reference
            shaderModuleState->AddUser();
            state->ownedShaderModules.push_back(shaderModuleState);
            state->referencedShaderModules.push_back(shaderModuleState);
        }

        // Collect all pipeline libraries
        if (auto* libraryCreateInfo = FindStructureTypeSafe<VkPipelineLibraryCreateInfoKHR>(&pCreateInfos[i], VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR)) {
            for (uint32_t libraryIndex = 0; libraryIndex < libraryCreateInfo->libraryCount; libraryIndex++) {
                PipelineState* libraryState = table->states_pipeline.Get(libraryCreateInfo->pLibraries[libraryIndex]);
                ASSERT(libraryState->ownedShaderModules.size() == libraryState->referencedShaderModules.size(), "Recursive libraries not supported");

                // Add all the shader modules of this library as referenced
                for (ShaderModuleState* shaderModuleState : libraryState->ownedShaderModules) {
                    state->referencedShaderModules.push_back(shaderModuleState);
                }
                
                // Add reference
                libraryState->AddUser();
                state->pipelineLibraries.push_back(libraryState);
            }
        }

        // Inform the controller
        table->instrumentationController->CreatePipelineAndAdd(state);
    }

    // Writeout
    std::memcpy(pPipelines, pipelines, sizeof(VkPipeline) * createInfoCount);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Delay writeout
    auto pipelines = ALLOCA_ARRAY(VkPipeline, createInfoCount);

    // Pass down callchain
    VkResult result = table->next_vkCreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pipelines);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Allocate states
    for (uint32_t i = 0; i < createInfoCount; i++) {
        auto state = new (table->allocators) ComputePipelineState;
        state->type = PipelineType::Compute;
        state->table = table;
        state->object = pipelines[i];
        state->isLibrary = (pCreateInfos[i].flags & VK_PIPELINE_CREATE_LIBRARY_BIT_KHR);
        state->createInfoDeepCopy.DeepCopy(table->allocators, pCreateInfos[i]);

        // External user
        state->AddUser();

        // Add a reference to the layout
        if (pCreateInfos[i].layout) {
            state->layout = table->states_pipelineLayout.Get(pCreateInfos[i].layout);
            state->layout->AddUser();
        } else {
            ASSERT(state->isLibrary, "Expected pipeline layout on non-library pipelines");
        }

        // Optional with libraries
        if (state->createInfoDeepCopy.createInfo.stage.module) {
            // Get the proxied shader state
            ShaderModuleState* shaderModuleState = GetPipelineStageShaderModule(table, state->createInfoDeepCopy.createInfo.stage);

            // Add reference
            shaderModuleState->AddUser();
            state->ownedShaderModules.push_back(shaderModuleState);
            state->referencedShaderModules.push_back(shaderModuleState);
        }

        // Collect all pipeline libraries
        if (auto* libraryCreateInfo = FindStructureTypeSafe<VkPipelineLibraryCreateInfoKHR>(&pCreateInfos[i], VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR)) {
            for (uint32_t libraryIndex = 0; libraryIndex < libraryCreateInfo->libraryCount; libraryIndex++) {
                PipelineState* libraryState = table->states_pipeline.Get(libraryCreateInfo->pLibraries[libraryIndex]);

                // Add all the shader modules of this library as referenced
                for (ShaderModuleState* shaderModuleState : libraryState->ownedShaderModules) {
                    state->referencedShaderModules.push_back(shaderModuleState);
                }

                // Add reference
                libraryState->AddUser();
                state->pipelineLibraries.push_back(libraryState);
            }
        }

        // Inform the controller
        table->instrumentationController->CreatePipelineAndAdd(state);
    }

    // Writeout
    std::memcpy(pPipelines, pipelines, sizeof(VkPipeline) * createInfoCount);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Delay writeout
    auto pipelines = ALLOCA_ARRAY(VkPipeline, createInfoCount);

    // Pass down callchain
    VkResult result = table->next_vkCreateRayTracingPipelinesKHR(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pipelines);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Allocate states
    for (uint32_t i = 0; i < createInfoCount; i++) {
        auto state = new (table->allocators) RaytracingPipelineState;
        state->type = PipelineType::Compute;
        state->table = table;
        state->object = pipelines[i];

        // External user
        state->AddUser();

        // Add a reference to the layout
        state->layout = table->states_pipelineLayout.Get(pCreateInfos[i].layout);
        state->layout->AddUser();

        // Inform the controller
        table->instrumentationController->CreatePipelineAndAdd(state);
    }

    // Writeout
    std::memcpy(pPipelines, pipelines, sizeof(VkPipeline) * createInfoCount);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Null destruction is allowed by the standard
    if (!pipeline) {
        return;
    }

    // Destroy the state
    PipelineState* state = table->states_pipeline.Get(pipeline);

    // The original shader module is now inaccessible
    //  ? To satisfy the pAllocator constraints, the original object must be released now
    state->object = nullptr;

    // Remove logical object from lookup
    //  Logical reference to state is invalid after this function
    table->states_pipeline.RemoveLogical(pipeline);

    // Release a reference to the object
    destroyRef(state, table->allocators);

    // Pass down callchain
    table->next_vkDestroyPipeline(device, pipeline, pAllocator);
}

PipelineState::~PipelineState() {
    // Type specific info
    switch (type) {
        default:
            break;
        case PipelineType::Graphics: {
            auto* graphics = static_cast<GraphicsPipelineState*>(this);

            // Free the render pass
            if (graphics->renderPass) {
                destroyRef(graphics->renderPass, table->allocators);
            }
            break;
        }
    }

    // Release all instrumented objects
    for (auto&& kv : instrumentObjects) {
        table->next_vkDestroyPipeline(table->object, kv.second, nullptr);
    }

    // Release all dependencies to the shader modules
    // All referenced modules are added as dependencies
    for (ShaderModuleState* module : referencedShaderModules) {
        table->dependencies_shaderModulesPipelines.Remove(module, this);
    }

    // Release all references to the shader modules
    // We only own those used during creation
    for (ShaderModuleState* module : ownedShaderModules) {
        destroyRef(module, table->allocators);
    }

    // Release all references to the pipeline libraries
    for (PipelineState* library : pipelineLibraries) {
        // Release dependency
        table->dependencies_pipelineLibraries.Remove(library, this);

        // Release ref
        destroyRef(library, table->allocators);
    }

    // Free the layout
    if (layout) {
        destroyRef(layout, table->allocators);
    }
}

void PipelineState::ReleaseHost() {
    // Remove state lookup
    // Reference host has locked this
    table->states_pipeline.RemoveStateNoLock(this);
}
