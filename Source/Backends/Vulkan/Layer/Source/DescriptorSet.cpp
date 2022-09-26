#include "Backends/Vulkan/States/PipelineLayoutState.h"
#include "Backends/Vulkan/States/ShaderModuleState.h"
#include "Backends/Vulkan/States/DescriptorSetLayoutState.h"
#include "Backends/Vulkan/Objects/CommandBufferObject.h"
#include "Backends/Vulkan/Tables/DeviceDispatchTable.h"
#include "Backends/Vulkan/Export/ShaderExportDescriptorAllocator.h"
#include "Backends/Vulkan/Export/ShaderExportStreamer.h"
#include <Backends/Vulkan/Translation.h>

// Common
#include <Common/Hash.h>

VKAPI_ATTR VkResult VKAPI_PTR Hook_vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create the new state
    auto state = new(table->allocators) DescriptorSetLayoutState;
    state->object = *pSetLayout;
    state->compatabilityHash = 0x0;

    // Hash
    CombineHash(state->compatabilityHash, pCreateInfo->bindingCount);

    // Check all binding types
    for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++) {
        switch (pCreateInfo->pBindings[i].descriptorType) {
            default:
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                state->dynamicOffsets++;
                break;
        }

        // Hash
        CombineHash(state->compatabilityHash, pCreateInfo->pBindings[i].descriptorType);
        CombineHash(state->compatabilityHash, pCreateInfo->pBindings[i].descriptorCount);
        CombineHash(state->compatabilityHash, pCreateInfo->pBindings[i].binding);
    }

    // Store lookup
    table->states_descriptorSetLayout.Add(*pSetLayout, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_PTR Hook_vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Null destruction is allowed by the standard
    if (!descriptorSetLayout) {
        return;
    }

    // Remove the state
    table->states_descriptorSetLayout.Remove(descriptorSetLayout);

    // Pass down callchain
    table->next_vkDestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pPipelineLayout) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // If we have exhausted all the sets, we can't add further records
    bool exhausted = pCreateInfo->setLayoutCount >= table->physicalDeviceProperties.limits.maxBoundDescriptorSets;
    if (exhausted) {
        // Pass down callchain
        VkResult result = table->next_vkCreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
        if (result != VK_SUCCESS) {
            return result;
        }
    } else {
        // Copy previous set layouts
        auto *setLayouts = ALLOCA_ARRAY(VkDescriptorSetLayout, pCreateInfo->setLayoutCount + 1);
        std::memcpy(setLayouts, pCreateInfo->pSetLayouts, sizeof(VkDescriptorSetLayout) * pCreateInfo->setLayoutCount);

        // Last set is export data
        setLayouts[pCreateInfo->setLayoutCount] = table->exportDescriptorAllocator->GetLayout();

        // Mirror creation info
        VkPipelineLayoutCreateInfo createInfo = *pCreateInfo;
        createInfo.setLayoutCount = pCreateInfo->setLayoutCount + 1;
        createInfo.pSetLayouts = setLayouts;

        // Pass down callchain
        VkResult result = table->next_vkCreatePipelineLayout(device, &createInfo, pAllocator, pPipelineLayout);
        if (result != VK_SUCCESS) {
            return result;
        }
    }

    // Create the new state
    auto state = new(table->allocators) PipelineLayoutState;
    state->table = table;
    state->object = *pPipelineLayout;
    state->exhausted = exhausted;
    state->descriptorDynamicOffsets.resize(pCreateInfo->setLayoutCount);
    state->compatabilityHashes.resize(pCreateInfo->setLayoutCount);

    // Copy set layout info
    for (uint32_t i = 0; i < pCreateInfo->setLayoutCount; i++) {
        DescriptorSetLayoutState* setLayoutState = table->states_descriptorSetLayout.Get(pCreateInfo->pSetLayouts[i]);

        // Inherit
        state->descriptorDynamicOffsets[i] = setLayoutState->dynamicOffsets;
        state->compatabilityHashes[i] = setLayoutState->compatabilityHash;

        // Combine layout hash
        CombineHash(state->compatabilityHash, setLayoutState->compatabilityHash);
    }

    // External user
    state->AddUser();

    // Number of bound descriptor sets
    state->boundUserDescriptorStates = pCreateInfo->setLayoutCount;

    // Store lookup
    table->states_pipelineLayout.Add(*pPipelineLayout, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_PTR Hook_vkCmdBindDescriptorSets(CommandBufferObject* commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets) {
    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdBindDescriptorSets(commandBuffer->object, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);

    // Debugging
#if TRACK_DESCRIPTOR_SETS
    for (uint32_t i = 0; i < descriptorSetCount; i++) {
        commandBuffer->context.descriptorSets[static_cast<uint32_t>(Translate(pipelineBindPoint))][firstSet + i] = pDescriptorSets[i];
    }
#endif

    // Inform streamer
    commandBuffer->table->exportStreamer->BindDescriptorSets(commandBuffer->streamState, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks *pAllocator) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Null destruction is allowed by the standard
    if (!pipelineLayout) {
        return;
    }

    // Get the state
    PipelineLayoutState *state = table->states_pipelineLayout.Get(pipelineLayout);

    /* Object deletion deferred to reference destruction */
    destroyRef(state, table->allocators);
}

PipelineLayoutState::~PipelineLayoutState() {
    // Remove the state
    table->states_pipelineLayout.Remove(object, this);

    // Pass down callchain
    table->next_vkDestroyPipelineLayout(table->object, object, nullptr);
}
