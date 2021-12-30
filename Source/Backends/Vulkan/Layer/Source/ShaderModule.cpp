#include <Backends/Vulkan/States/ShaderModuleState.h>
#include <Backends/Vulkan/ShaderModule.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Allocate state
    auto state =  table->states_shaderModule.Add(*pShaderModule, new (table->allocators) ShaderModuleState);
    state->table = table;
    state->object = *pShaderModule;

    // Create a deep copy
    state->createInfoDeepCopy.DeepCopy(table->allocators, *pCreateInfo);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Destroy the state
    ShaderModuleState* state = table->states_shaderModule.Get(shaderModule);

    // The original shader module is now inaccessible
    //  ? To satisfy the pAllocator constraints, the original object must be released now
    state->object = nullptr;

    // Remove logical object from lookup
    //  Logical reference to state is invalid after this function
    table->states_shaderModule.RemoveLogical(shaderModule);

    // Release a reference to the object
    destroyRef(state, table->allocators);

    // Pass down callchain
    table->next_vkDestroyShaderModule(device, shaderModule, pAllocator);
}

ShaderModuleState::~ShaderModuleState() {
    // Remove state lookup
    table->states_shaderModule.RemoveState(this);

    // If there's any dangling dependencies, someone forgot to add a reference
    ASSERT(table->dependencies_shaderModulesPipelines.Count(this) == 0, "Dangling pipeline references");
}
