#include "Backends/Vulkan/States/RenderPassState.h"
#include "Backends/Vulkan/Tables/DeviceDispatchTable.h"

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create the new state
    auto state = new(table->allocators) RenderPassState;
    state->table = table;
    state->object = *pRenderPass;

    // External user
    state->AddUser();

    // Store lookup
    table->states_renderPass.Add(*pRenderPass, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Get the state
    RenderPassState *state = table->states_renderPass.Get(renderPass);

    /* Object deletion deferred to reference destruction */
    destroyRef(state, table->allocators);
}

RenderPassState::~RenderPassState() {
    // Remove the state
    table->states_renderPass.Remove(object, this);

    // Pass down callchain
    table->next_vkDestroyRenderPass(table->object, object, nullptr);
}
