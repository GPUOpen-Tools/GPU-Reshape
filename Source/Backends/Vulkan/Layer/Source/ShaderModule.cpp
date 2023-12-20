// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

#include <Backends/Vulkan/States/ShaderModuleState.h>
#include <Backends/Vulkan/ShaderModule.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Compiler/SpvModule.h>

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Allocate state
    auto state = table->states_shaderModule.Add(*pShaderModule, new (table->allocators) ShaderModuleState);
    state->table = table;
    state->object = *pShaderModule;

    // External user
    state->AddUser();

    // Create a deep copy
    state->createInfoDeepCopy.DeepCopy(table->allocators, *pCreateInfo);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Null destruction is allowed by the standard
    if (!shaderModule) {
        return;
    }

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

    // Release instrumented modules
    for (auto&& kv : instrumentObjects) {
        table->next_vkDestroyShaderModule(table->object, kv.second, nullptr);
    }

    // Release spirv module
    if (spirvModule) {
        destroy(spirvModule, table->allocators);
    }

    // If there's any dangling dependencies, someone forgot to add a reference
    ASSERT(table->dependencies_shaderModulesPipelines.Count(this) == 0, "Dangling pipeline references");
}
