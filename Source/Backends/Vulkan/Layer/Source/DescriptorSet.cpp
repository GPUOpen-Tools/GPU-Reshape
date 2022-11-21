#include "Backends/Vulkan/States/PipelineLayoutState.h"
#include "Backends/Vulkan/States/ShaderModuleState.h"
#include "Backends/Vulkan/States/DescriptorSetLayoutState.h"
#include "Backends/Vulkan/States/DescriptorSetState.h"
#include "Backends/Vulkan/States/BufferState.h"
#include "Backends/Vulkan/States/SamplerState.h"
#include "Backends/Vulkan/States/ImageState.h"
#include "Backends/Vulkan/Objects/CommandBufferObject.h"
#include "Backends/Vulkan/Tables/DeviceDispatchTable.h"
#include "Backends/Vulkan/Export/ShaderExportDescriptorAllocator.h"
#include "Backends/Vulkan/Export/ShaderExportStreamer.h"
#include <Backends/Vulkan/Translation.h>
#include <Backends/Vulkan/Resource/PhysicalResourceMappingTable.h>

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
    state->descriptorCount = pCreateInfo->bindingCount;

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

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkAllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create the new states
    for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
        auto state = new(table->allocators) DescriptorSetState;
        state->object = pDescriptorSets[i];
        state->table = table;

        // Get layout
        const DescriptorSetLayoutState* layout = table->states_descriptorSetLayout.Get(pAllocateInfo->pSetLayouts[i]);

        // Allocate segment for given layout
        state->segmentID = table->prmTable->Allocate(layout->descriptorCount);

        // Store lookup
        table->states_descriptorSet.Add(pDescriptorSets[i], state);
    }

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Remove the states
    for (uint32_t i = 0; i < descriptorSetCount; i++) {
        table->states_descriptorSet.Remove(pDescriptorSets[i]);
    }

    // Pass down callchain
    return table->next_vkFreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Create PRM associations from writes
    for (uint32_t i = 0; i < descriptorWriteCount; i++) {
        const VkWriteDescriptorSet& write = pDescriptorWrites[i];

        // Get the originating set
        const DescriptorSetState* state = table->states_descriptorSet.Get(write.dstSet);

        // Create mappings for all descriptors written
        for (uint32_t descriptorIndex = 0; descriptorIndex < write.descriptorCount; descriptorIndex++) {
            VirtualResourceMapping mapping;
            switch (write.descriptorType) {
                default:
                    // Perhaps handled in the future
                    continue;
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                    mapping = table->states_sampler.Get(write.pImageInfo[descriptorIndex].sampler)->virtualMapping;
                    break;
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    mapping = table->states_imageView.Get(write.pImageInfo[descriptorIndex].imageView)->virtualMapping;
                    break;
                case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                    mapping = table->states_bufferView.Get(write.pTexelBufferView[descriptorIndex])->virtualMapping;
                    break;
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                    mapping = table->states_buffer.Get(write.pBufferInfo[descriptorIndex].buffer)->virtualMapping;
                    break;
            }

            // Update the table
            table->prmTable->WriteMapping(state->segmentID, write.dstBinding + write.dstArrayElement, mapping);
        }
    }

    // Create PRM associations from copies
    for (uint32_t i = 0; i < descriptorCopyCount; i++) {
        const VkCopyDescriptorSet& copy = pDescriptorCopies[i];

        // Get the sets
        const DescriptorSetState* stateSrc = table->states_descriptorSet.Get(copy.srcSet);
        const DescriptorSetState* stateDst = table->states_descriptorSet.Get(copy.dstSet);

        // Create mappings for all descriptors copied
        for (uint32_t descriptorIndex = 0; descriptorIndex < copy.descriptorCount; descriptorIndex++) {
            // Get the original mapping
            VirtualResourceMapping srcMapping = table->prmTable->GetMapping(stateSrc->segmentID, copy.srcBinding + copy.srcArrayElement);

            // Write as new mapping
            table->prmTable->WriteMapping(stateDst->segmentID, copy.dstBinding + copy.dstArrayElement, srcMapping);
        }
    }

    // Pass down callchain
    table->next_vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
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
