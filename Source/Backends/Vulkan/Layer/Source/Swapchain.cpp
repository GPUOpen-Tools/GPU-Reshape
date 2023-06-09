#include <Backends/Vulkan/Swapchain.h>
#include <Backends/Vulkan/States/SwapchainState.h>
#include <Backends/Vulkan/States/ImageState.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>

// Backend
#include <Backend/IL/ResourceTokenType.h>

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
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
            imageState->virtualMappingTemplate.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Texture);
            imageState->virtualMappingTemplate.puid = table->physicalResourceIdentifierMap.AllocatePUID();
            imageState->virtualMappingTemplate.srb = ~0u;

            // Store lookup
            table->states_image.Add(imageState->object, imageState);
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
        table->physicalResourceIdentifierMap.FreePUID(imageState->virtualMappingTemplate.puid);

        // Remove the state
        table->states_image.Remove(imageState->object);
    }

    // Remove the state
    table->states_swapchain.Remove(swapchain);

    // Pass down callchain
    table->next_vkDestroySwapchainKHR(device, swapchain, pAllocator);
}
