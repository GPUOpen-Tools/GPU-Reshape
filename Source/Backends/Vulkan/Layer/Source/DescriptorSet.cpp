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

#include "Backends/Vulkan/States/PipelineLayoutState.h"
#include "Backends/Vulkan/States/ShaderModuleState.h"
#include "Backends/Vulkan/States/DescriptorSetLayoutState.h"
#include "Backends/Vulkan/States/DescriptorSetState.h"
#include "Backends/Vulkan/States/BufferState.h"
#include "Backends/Vulkan/States/SamplerState.h"
#include "Backends/Vulkan/States/ImageState.h"
#include <Backends/Vulkan/States/DescriptorUpdateTemplateState.h>
#include "Backends/Vulkan/Objects/CommandBufferObject.h"
#include "Backends/Vulkan/Tables/DeviceDispatchTable.h"
#include "Backends/Vulkan/Export/ShaderExportDescriptorAllocator.h"
#include "Backends/Vulkan/Export/ShaderExportStreamer.h"
#include <Backends/Vulkan/Translation.h>
#include <Backends/Vulkan/Resource/PhysicalResourceMappingTable.h>
#include <Backends/Vulkan/ShaderData/ShaderDataHost.h>
#include <Backends/Vulkan/States/DescriptorPoolState.h>
#include <Backends/Vulkan/Resource/DescriptorResourceMapping.h>

// Backend
#include <Backend/IL/ResourceTokenType.h>

// Common
#include <Common/Hash.h>

VKAPI_ATTR VkResult VKAPI_PTR Hook_vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Find optional extensions
    const auto* bindingFlagsEXT = FindStructureTypeSafe<VkDescriptorSetLayoutBindingFlagsCreateInfoEXT>(pCreateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT);
    
    // Create the new state
    auto state = new(table->allocators) DescriptorSetLayoutState;
    state->object = *pSetLayout;
    state->compatabilityHash = 0x0;
    
    // Hash
    CombineHash(state->compatabilityHash, pCreateInfo->bindingCount);

    // Total number of bindings
    uint32_t bindingCount = 0;
    
    // Check all binding types
    for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++) {
        const VkDescriptorSetLayoutBinding& binding = pCreateInfo->pBindings[i];
        switch (binding.descriptorType) {
            default:
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                state->dynamicOffsets++;
                break;
        }

        // Hash
        CombineHash(state->compatabilityHash, binding.descriptorType);
        CombineHash(state->compatabilityHash, binding.descriptorCount);
        CombineHash(state->compatabilityHash, binding.binding);

        // Cache counts
        bindingCount = std::max(bindingCount, binding.binding + 1u);
    }

    // Reserve mappings
    state->physicalMapping.bindings.resize(bindingCount);
    
    // Cache counts
    for (uint32_t i = 0; i < pCreateInfo->bindingCount; i++) {
        const VkDescriptorSetLayoutBinding& binding = pCreateInfo->pBindings[i];

        // Update mapping
        BindingPhysicalMapping& mapping = state->physicalMapping.bindings[binding.binding];
        mapping.type = binding.descriptorType;
        mapping.immutableSamplers = binding.pImmutableSamplers != nullptr;
        mapping.bindingCount = binding.descriptorCount;
        
        // Keep optional flags
        if (bindingFlagsEXT) {
            mapping.flags = bindingFlagsEXT->pBindingFlags[i];
        }
    }

    // Accumulate offsets
    for (uint32_t i = 1; i < bindingCount; i++) {
        const BindingPhysicalMapping& last = state->physicalMapping.bindings[i - 1u];

        // Update offset
        BindingPhysicalMapping& current = state->physicalMapping.bindings[i];
        current.prmtOffset = last.prmtOffset + last.bindingCount;
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

    // Find optional extensions
    const auto* variableCountEXT = FindStructureTypeSafe<VkDescriptorSetVariableDescriptorCountAllocateInfoEXT>(pAllocateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT);

    // Get pool
    DescriptorPoolState* poolState = table->states_descriptorPool.Get(pAllocateInfo->descriptorPool);

    // Create the new states
    for (uint32_t setIndex = 0; setIndex < pAllocateInfo->descriptorSetCount; setIndex++) {
        // Create state
        auto state = new(table->allocators) DescriptorSetState;
        state->object = pDescriptorSets[setIndex];
        state->table = table;
        state->poolSwapIndex = static_cast<uint32_t>(poolState->states.size());
        state->descriptorCount = 0;

        // Add state object
        poolState->states.push_back(state);
        
        // Get layout
        const DescriptorSetLayoutState* layout = table->states_descriptorSetLayout.Get(pAllocateInfo->pSetLayouts[setIndex]);

        // Accumulate offsets
        for (uint32_t bindingIdx = 0; bindingIdx < layout->physicalMapping.bindings.size(); bindingIdx++) {
            const BindingPhysicalMapping& mapping = layout->physicalMapping.bindings[bindingIdx];

            // Default count from layout
            uint32_t bindingCount = mapping.bindingCount;

            // Is variable counts?
            if (mapping.flags & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT) {
                if (variableCountEXT) {
                    // Get from reported counts
                    bindingCount = variableCountEXT->pDescriptorCounts[setIndex];
                } else {
                    // "If descriptorSetCount is zero or this structure is not included in the pNext chain, then the variable lengths are considered to be zero."
                    bindingCount = 0;
                }
            }

            // Update count
            state->descriptorCount += bindingCount;
        }
        
        // Allocate segment for given layout
        state->segmentID = table->prmTable->Allocate(state->descriptorCount);

        // Preallocate
        state->prmtOffsets.reserve(layout->physicalMapping.bindings.size());

        // Prepare bindings
        for (const BindingPhysicalMapping& mapping : layout->physicalMapping.bindings) {
            // Copy PRMT offset
            state->prmtOffsets.push_back(mapping.prmtOffset);

            // Immutable exclusive samplers?
            if (mapping.immutableSamplers && mapping.type == VK_DESCRIPTOR_TYPE_SAMPLER) {
                // Prepare mapping
                VirtualResourceMapping virtualMapping;
                virtualMapping.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Sampler);
                virtualMapping.puid = IL::kResourceTokenPUIDReservedNullSampler;
            
                // Update the table with immutable samplers
                for (uint32_t descriptorIndex = 0; descriptorIndex < mapping.bindingCount; descriptorIndex++) {
                    table->prmTable->WriteMapping(state->segmentID, mapping.prmtOffset + descriptorIndex, virtualMapping);
                }
            }
        }

        // Store lookup
        table->states_descriptorSet.Add(pDescriptorSets[setIndex], state);
    }

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Get pool
    DescriptorPoolState* pool = table->states_descriptorPool.Get(descriptorPool);

    // Remove the states
    for (uint32_t i = 0; i < descriptorSetCount; i++) {
        // Null destruction is allowed by the standard
        if (!pDescriptorSets[i]) {
            continue;
        }

        // Get stata
        DescriptorSetState* setState = table->states_descriptorSet.Get(pDescriptorSets[i]);

        // Not last element?
        if (setState->poolSwapIndex != pool->states.size() - 1) {
            DescriptorSetState* lastState = pool->states.back();

            // Swap last with current
            pool->states[setState->poolSwapIndex] = lastState;

            // Reassign new position of last
            lastState->poolSwapIndex = setState->poolSwapIndex;
        }

        // Remove state
        pool->states.pop_back();
        
        // Destroy the PRMT segment range
        table->prmTable->Free(setState->segmentID);

        // Remove tracking
        table->states_descriptorSet.Remove(setState->object, setState);

        // Destroy state
        destroy(setState, table->allocators);
    }

    // Pass down callchain
    return table->next_vkFreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
    if (result != VK_SUCCESS) {
        return result;
    }
    
    auto state = new(table->allocators) DescriptorPoolState;
    state->object = *pDescriptorPool;
    state->table = table;

    // Store lookup
    table->states_descriptorPool.Add(*pDescriptorPool, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));
    
    // Get pool
    DescriptorPoolState* pool = table->states_descriptorPool.Get(descriptorPool);
    
    // Remove the states
    for (DescriptorSetState* state : pool->states) {
        // Destroy the PRMT segment range
        table->prmTable->Free(state->segmentID);

        // Remove tracking
        table->states_descriptorSet.Remove(state->object, state);

        // Destroy state
        destroy(state, table->allocators);
    }

    // Cleanup
    pool->states.clear();

    // Pass down callchain
    return table->next_vkResetDescriptorPool(device, descriptorPool, flags);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Null destruction is allowed by the standard
    if (!descriptorPool) {
        return;
    }
    
    // Get pool
    DescriptorPoolState* pool = table->states_descriptorPool.Get(descriptorPool);

    // Remove the states
    for (DescriptorSetState* state : pool->states) {
        // Destroy the PRMT segment range
        table->prmTable->Free(state->segmentID);

        // Remove tracking
        table->states_descriptorSet.Remove(state->object, state);

        // Destroy state
        destroy(state, table->allocators);
    }
    
    // Pass down callchain
    table->next_vkDestroyDescriptorPool(device, descriptorPool, pAllocator);
    
    // Store lookup
    table->states_descriptorPool.Remove(descriptorPool);
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
            // Default invalid mapping
            VirtualResourceMapping mapping = GetVirtualResourceMapping(table, write, descriptorIndex);
            
            // Map current binding to an offset
            const uint32_t prmtOffset = state->prmtOffsets.at(write.dstBinding);

            // Update the table
            table->prmTable->WriteMapping(state->segmentID, prmtOffset + write.dstArrayElement + descriptorIndex, mapping);
        }
    }

    // Create PRM associations from copies
    for (uint32_t i = 0; i < descriptorCopyCount; i++) {
        const VkCopyDescriptorSet& copy = pDescriptorCopies[i];

        // Get the sets
        const DescriptorSetState* stateSrc = table->states_descriptorSet.Get(copy.srcSet);
        const DescriptorSetState* stateDst = table->states_descriptorSet.Get(copy.dstSet);

        // Map bindings to offsets
        const uint32_t srcPrmtOffset = stateSrc->prmtOffsets.at(copy.srcBinding);
        const uint32_t dstPrmtOffset = stateDst->prmtOffsets.at(copy.dstBinding);

        // Create mappings for all descriptors copied
        for (uint32_t descriptorIndex = 0; descriptorIndex < copy.descriptorCount; descriptorIndex++) {
            // Get the original mapping
            VirtualResourceMapping srcMapping = table->prmTable->GetMapping(stateSrc->segmentID, srcPrmtOffset + copy.srcArrayElement + descriptorIndex);

            // Write as new mapping
            table->prmTable->WriteMapping(stateDst->segmentID, dstPrmtOffset + copy.dstArrayElement + descriptorIndex, srcMapping);
        }
    }

    // Pass down callchain
    table->next_vkUpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

VKAPI_ATTR VkResult VKAPI_PTR Hook_vkCreateDescriptorUpdateTemplate(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorUpdateTemplate* pDescriptorUpdateTemplate) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateDescriptorUpdateTemplate(device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
    if (result != VK_SUCCESS) {
        return result;
    }
    
    auto state = new(table->allocators) DescriptorUpdateTemplateState;
    state->object = *pDescriptorUpdateTemplate;
    state->table = table;

    // Perform deep copy
    state->createInfo.DeepCopy(table->allocators, *pCreateInfo);

    // Store lookup
    table->states_descriptorUpdateTemplateState.Add(*pDescriptorUpdateTemplate, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_PTR Hook_vkDestroyDescriptorUpdateTemplate(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Null destruction is allowed by the standard
    if (!descriptorUpdateTemplate) {
        return;
    }
    
    // Get pool
    DescriptorUpdateTemplateState* state = table->states_descriptorUpdateTemplateState.Get(descriptorUpdateTemplate);
    
    // Pass down callchain
    table->next_vkDestroyDescriptorUpdateTemplate(device, descriptorUpdateTemplate, pAllocator);
    
    // Remove lookup
    table->states_descriptorUpdateTemplateState.Remove(descriptorUpdateTemplate);

    // Cleanup
    destroy(state, table->allocators);
}

VKAPI_ATTR void VKAPI_PTR Hook_vkUpdateDescriptorSetWithTemplate(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void* pData) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));
    
    // Pass down callchain
    table->next_vkUpdateDescriptorSetWithTemplate(device, descriptorSet, descriptorUpdateTemplate, pData);

    // Get states
    const DescriptorUpdateTemplateState* templateState = table->states_descriptorUpdateTemplateState.Get(descriptorUpdateTemplate);
    const DescriptorSetState*            setState      = table->states_descriptorSet.Get(descriptorSet);

    // Handle each entry
    for (uint32_t i = 0; i < templateState->createInfo->descriptorUpdateEntryCount; i++) {
        const VkDescriptorUpdateTemplateEntry& entry = templateState->createInfo->pDescriptorUpdateEntries[i];

        // Handle each binding write
        for (uint32_t descriptorIndex = 0; descriptorIndex < entry.descriptorCount; descriptorIndex++) {
            const void *descriptorData = static_cast<const uint8_t*>(pData) + entry.offset + descriptorIndex * entry.stride;

            // Get mapping
            VirtualResourceMapping mapping = GetVirtualResourceMapping(table, entry.descriptorType, descriptorData);

            // Map current binding to an offset
            const uint32_t prmtOffset = setState->prmtOffsets.at(entry.dstBinding);

            // Update the table
            table->prmTable->WriteMapping(setState->segmentID, prmtOffset + entry.dstArrayElement + descriptorIndex, mapping);
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pPipelineLayout) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // User push constant offset
    uint32_t userPushConstantLength = 0;
#if PRMT_METHOD == PRMT_METHOD_UB_PC
    uint32_t prmtPushConstantOffset = 0;
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC
    uint32_t dataPushConstantOffset = 0;
    uint32_t dataPushConstantLength = 0;
    VkShaderStageFlags pushConstantRangeMask = 0;

    // The vulkan specification provides no guarantees on allocation lifetimes *beyond* destruction.
    // So, we cannot safely keep the handles around. Use the internal allocators instead.
    pAllocator = nullptr;

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

        // Get number of events
        uint32_t eventCount{0};
        table->dataHost->Enumerate(&eventCount, nullptr, ShaderDataType::Event);

        // To length
        dataPushConstantLength += eventCount * sizeof(uint32_t);

        // Summarize user PCs
        for (uint32_t i = 0; i < pCreateInfo->pushConstantRangeCount; i++) {
            const VkPushConstantRange& userRange = pCreateInfo->pPushConstantRanges[i];

            // Mask all usages
            pushConstantRangeMask |= userRange.stageFlags;

            // Extend length
            userPushConstantLength = std::max(userPushConstantLength, userRange.offset + userRange.size);
        }

#if PIPELINE_MERGE_PC_RANGES
        // Create a merged range
        VkPushConstantRange mergedRange{};
        mergedRange.offset = 0;
        mergedRange.stageFlags = VK_SHADER_STAGE_ALL;
#else // PIPELINE_MERGE_PC_RANGES
        // Copy previous ranges
        auto *ranges = ALLOCA_ARRAY(VkPushConstantRange, pCreateInfo->pushConstantRangeCount + 1);
        std::memcpy(ranges, pCreateInfo->pPushConstantRanges, sizeof(VkPushConstantRange) * pCreateInfo->pushConstantRangeCount);
#endif // PIPELINE_MERGE_PC_RANGES

        // Mirror creation info
        VkPipelineLayoutCreateInfo createInfo = *pCreateInfo;
        createInfo.setLayoutCount = pCreateInfo->setLayoutCount + 1;
        createInfo.pSetLayouts = setLayouts;
#if PIPELINE_MERGE_PC_RANGES
        createInfo.pPushConstantRanges = &mergedRange;
        createInfo.pushConstantRangeCount = 1u;
#else // PIPELINE_MERGE_PC_RANGES
        createInfo.pPushConstantRanges = ranges;
#endif // PIPELINE_MERGE_PC_RANGES

        // Instrumented length
        uint32_t extendedPushConstantLength = userPushConstantLength;

#if PRMT_METHOD == PRMT_METHOD_UB_PC
        // Take single dword for PRMT sub-segment offset
        prmtPushConstantOffset = extendedPushConstantLength;
        extendedPushConstantLength += sizeof(uint32_t);
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC

        // Allocate extended length
        dataPushConstantOffset = extendedPushConstantLength;
        extendedPushConstantLength += dataPushConstantLength;

#if PIPELINE_MERGE_PC_RANGES
        // Extend merged range
        mergedRange.size = extendedPushConstantLength;
#else // PIPELINE_MERGE_PC_RANGES
        // Any data?
        if (extendedPushConstantLength > userPushConstantLength) {
            VkPushConstantRange* range{nullptr};

            // Existing range?
            for (uint32_t i = 0; i < pCreateInfo->pushConstantRangeCount; i++) {
                if (ranges[i].stageFlags == VK_SHADER_STAGE_ALL) {
                    range = ranges + i;
                }
            }
            
            // Append internal PC range
            if (!range) {
                range = ranges + pCreateInfo->pushConstantRangeCount;
                range->offset = userPushConstantLength;
                range->size = extendedPushConstantLength - userPushConstantLength;
                range->stageFlags = VK_SHADER_STAGE_ALL;
                createInfo.pushConstantRangeCount = pCreateInfo->pushConstantRangeCount + 1;

                // Use entire range
                pushConstantRangeMask |= VK_SHADER_STAGE_ALL;
            } else {
                range->size = extendedPushConstantLength - range->offset;
            }
        }
#endif // PIPELINE_MERGE_PC_RANGES

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
    state->physicalMapping.descriptorSets.resize(pCreateInfo->setLayoutCount);

    // Copy set layout info
    for (uint32_t i = 0; i < pCreateInfo->setLayoutCount; i++) {
        DescriptorSetLayoutState* setLayoutState = table->states_descriptorSetLayout.Get(pCreateInfo->pSetLayouts[i]);

        // Inherit
        state->descriptorDynamicOffsets[i] = setLayoutState->dynamicOffsets;
        state->compatabilityHashes[i] = setLayoutState->compatabilityHash;

        // Copy set layout physical mappings
        state->physicalMapping.descriptorSets[i] = setLayoutState->physicalMapping;

        // Combine layout hash
        CombineHash(state->compatabilityHash, setLayoutState->compatabilityHash);
    }

    // Inherit compatability hash
    state->physicalMapping.layoutHash = state->compatabilityHash;

    // External user
    state->AddUser();

    // Binding info
    state->boundUserDescriptorStates = pCreateInfo->setLayoutCount;
    state->userPushConstantLength = userPushConstantLength;
#if PRMT_METHOD == PRMT_METHOD_UB_PC
    state->prmtPushConstantOffset = prmtPushConstantOffset;
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC
    state->dataPushConstantOffset = dataPushConstantOffset;
    state->dataPushConstantLength = dataPushConstantLength;
    state->pushConstantRangeMask  = pushConstantRangeMask;

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
    commandBuffer->table->exportStreamer->BindDescriptorSets(commandBuffer->streamState, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets, commandBuffer->object);
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
