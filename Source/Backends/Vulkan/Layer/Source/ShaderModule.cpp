#include <Backends/Vulkan/ShaderModuleState.h>
#include <Backends/Vulkan/ShaderModule.h>
#include <Backends/Vulkan/DeviceDispatchTable.h>

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Allocate state
    auto state =  table->states_shaderModule.Add(*pShaderModule, new (table->allocators) ShaderModuleState);
    state->object = *pShaderModule;

    // Create a deep copy
    state->createInfoDeepCopy.DeepCopy(table->allocators, *pCreateInfo);

    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Destroy the state
    ShaderModuleState* state = table->states_shaderModule.Get(shaderModule);
    destroyRef(state, table->allocators);

    // Pass down callchain
    table->next_vkDestroyShaderModule(device, shaderModule, pAllocator);
}
