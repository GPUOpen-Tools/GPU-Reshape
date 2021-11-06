#include <Backends/Vulkan/PipelineState.h>
#include <Backends/Vulkan/Allocators.h>
#include <Backends/Vulkan/DeviceDispatchTable.h>

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Allocate states
    for (uint32_t i = 0; i < createInfoCount; i++) {
        auto state = new (table->allocators) GraphicsPipelineState;
        state->type = PipelineType::Graphics;
        state->object = pPipelines[i];
        state->createInfoDeepCopy.DeepCopy(table->allocators, pCreateInfos[i]);
        table->states_pipeline.Add(pPipelines[i], state);
    }

    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Allocate states
    for (uint32_t i = 0; i < createInfoCount; i++) {
        auto state = new (table->allocators) ComputePipelineState;
        state->type = PipelineType::Compute;
        state->object = pPipelines[i];
        state->createInfoDeepCopy.DeepCopy(table->allocators, pCreateInfos[i]);
        table->states_pipeline.Add(pPipelines[i], state);
    }

    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Destroy the state
    PipelineState* state = table->states_pipeline.Get(pipeline);
    destroyRef(state, table->allocators);

    // Pass down callchain
    table->next_vkDestroyPipeline(device, pipeline, pAllocator);
}
