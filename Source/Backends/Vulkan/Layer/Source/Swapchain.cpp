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

#include <Backends/Vulkan/Swapchain.h>
#include <Backends/Vulkan/Queue.h>
#include <Backends/Vulkan/States/SwapchainState.h>
#include <Backends/Vulkan/States/ImageState.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Resource.h>
#include <Backends/Vulkan/Translation.h>

// Backend
#include <Backend/IL/ResourceTokenType.h>

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Redirect all queue families
    auto* queueFamilies = ALLOCA_ARRAY(uint32_t, pCreateInfo->queueFamilyIndexCount);
    for (uint32_t i = 0; i < pCreateInfo->queueFamilyIndexCount; i++) {
        queueFamilies[i] = RedirectQueueFamily(table, pCreateInfo->pQueueFamilyIndices[i]);
    }

    // Create copy
    VkSwapchainCreateInfoKHR createInfo = *pCreateInfo;
    createInfo.pQueueFamilyIndices = queueFamilies;

    // Pass down callchain
    VkResult result = table->next_vkCreateSwapchainKHR(device, &createInfo, pAllocator, pSwapchain);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create state
    auto state = new(table->allocators) SwapchainState;
    state->object = *pSwapchain;
    state->table = table;

    // Get actual number of images
    uint32_t swapchainImages{0};
    if (result = table->next_vkGetSwapchainImagesKHR(device, state->object, &swapchainImages, nullptr); result != VK_SUCCESS) {
        return result;
    }

    // Pool images
    auto* images = ALLOCA_ARRAY(VkImage, pCreateInfo->minImageCount);
    if (result = table->next_vkGetSwapchainImagesKHR(device, state->object, &swapchainImages, images); result != VK_SUCCESS) {
        return result;
    }

    // Create image states
    for (uint32_t i = 0; i < pCreateInfo->minImageCount; i++) {
        ImageState* imageState = table->states_image.TryGet(images[i]);

        // Fresh swapchain?
        if (!imageState) {
            // Create new object
            imageState = new(table->allocators) ImageState;
            imageState->object = images[i];
            imageState->table = table;

            // Create mapping template
            imageState->virtualMappingTemplate.token.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Texture);
            imageState->virtualMappingTemplate.token.puid = table->physicalResourceIdentifierMap.AllocatePUID();
            imageState->virtualMappingTemplate.token.formatId = static_cast<uint32_t>(Translate(pCreateInfo->imageFormat));
            imageState->virtualMappingTemplate.token.formatSize = GetFormatByteSize(pCreateInfo->imageFormat);
            imageState->virtualMappingTemplate.token.width = pCreateInfo->imageExtent.width;
            imageState->virtualMappingTemplate.token.height = pCreateInfo->imageExtent.height;
            imageState->virtualMappingTemplate.token.depthOrSliceCount = pCreateInfo->imageArrayLayers;
            imageState->virtualMappingTemplate.token.DefaultViewToRange();

            // Store lookup
            table->states_image.Add(imageState->object, imageState);

            // Invoke proxies for all handles
            for (const FeatureHookTable &proxyTable: table->featureHookTables) {
                proxyTable.createResource.TryInvoke(GetResourceInfoFor(imageState));
            }
        }

        // Keep internal track
        state->imageStates.push_back(imageState);

        // Reassign owner
        imageState->owningHandle = reinterpret_cast<uint64_t>(*pSwapchain);
    }

    // Store lookup
    table->states_swapchain.Add(*pSwapchain, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Specification allows null destruction
    if (!swapchain) {
        return;
    }

    // Get swapchain
    SwapchainState* state = table->states_swapchain.Get(swapchain);

    // Free all images
    for (ImageState* imageState : state->imageStates) {
        // May no longer be the owner if the swapchain was recreated
        if (imageState->owningHandle != reinterpret_cast<uint64_t>(swapchain)) {
            continue;
        }
        
        // Release the token
        table->physicalResourceIdentifierMap.FreePUID(imageState->virtualMappingTemplate.token.puid);

        // Remove the state
        table->states_image.Remove(imageState->object);
    }

    // Remove the state
    table->states_swapchain.Remove(swapchain);

    // Pass down callchain
    table->next_vkDestroySwapchainKHR(device, swapchain, pAllocator);
}
