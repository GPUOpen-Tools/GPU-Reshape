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
        state->createInfoDeepCopy.DeepCopy(table->allocators, pCreateInfos[i]);

        // External user
        state->AddUser();

        // Add a reference to the layout
        state->layout = table->states_pipelineLayout.Get(pCreateInfos[i].layout);
        state->layout->AddUser();

        // Add reference to the render pass
        if (pCreateInfos[i].renderPass) {
            state->renderPass = table->states_renderPass.Get(pCreateInfos[i].renderPass);
            state->renderPass->AddUser();
        }

        // Collect all shader modules
        for (uint32_t stageIndex = 0; stageIndex < state->createInfoDeepCopy.createInfo.stageCount; stageIndex++) {
            const VkPipelineShaderStageCreateInfo& stageInfo = state->createInfoDeepCopy.createInfo.pStages[stageIndex];

            // Get the proxied state
            ShaderModuleState* shaderModuleState = table->states_shaderModule.Get(stageInfo.module);

            // Add reference
            shaderModuleState->AddUser();
            state->shaderModules.push_back(shaderModuleState);
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
        state->createInfoDeepCopy.DeepCopy(table->allocators, pCreateInfos[i]);

        // External user
        state->AddUser();

        // Add a reference to the layout
        state->layout = table->states_pipelineLayout.Get(pCreateInfos[i].layout);
        state->layout->AddUser();

        // Get the proxied shader state
        ShaderModuleState* shaderModuleState = table->states_shaderModule.Get(state->createInfoDeepCopy.createInfo.stage.module);

        // Add reference
        shaderModuleState->AddUser();
        state->shaderModules.push_back(shaderModuleState);

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
    // Remove state lookup
    table->states_pipeline.RemoveState(this);

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

    // Release all references to the shader modules
    for (ShaderModuleState* module : shaderModules) {
        // Release dependency
        table->dependencies_shaderModulesPipelines.Remove(module, this);

        // Release ref
        destroyRef(module, table->allocators);
    }

    // Free the layout
    destroyRef(layout, table->allocators);
}
