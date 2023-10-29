// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

#include <Backends/Vulkan/Instance.h>
#include <Backends/Vulkan/Layer.h>
#include <Backends/Vulkan/DeepCopyObjects.Gen.h>
#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>
#include <Backends/Vulkan/Compiler/ShaderCompilerDebug.h>

// Common
#include "Common/Dispatcher/Dispatcher.h"
#include <Common/Registry.h>
#include <Common/CrashHandler.h>

// Backend
#include <Backend/EnvironmentInfo.h>
#include <Backend/IFeatureHost.h>
#include <Backend/IFeature.h>

// Vulkan
#include <vulkan/vk_layer.h>

// Std
#include <cstring>

// Bridge
#include <Bridge/IBridge.h>

/// Find a structure type
template<typename T, typename U>
inline const T *FindStructureType(const U *chain, uint64_t type) {
    const U *current = chain;
    while (current && current->sType != type) {
        current = reinterpret_cast<const U *>(current->pNext);
    }

    return reinterpret_cast<const T *>(current);
}

VkResult VKAPI_PTR Hook_vkEnumerateInstanceLayerProperties(uint32_t *pPropertyCount, VkLayerProperties *pProperties) {
    if (pPropertyCount) {
        *pPropertyCount = 1;
    }

    if (pProperties) {
        strcpy_s(pProperties->layerName, "VK_LAYER_GPUOPEN_GRS");
        strcpy_s(pProperties->description, "");
        pProperties->implementationVersion = 1;
        pProperties->specVersion = VK_API_VERSION_1_0;
    }

    return VK_SUCCESS;
}

VkResult VKAPI_PTR Hook_vkEnumerateInstanceExtensionProperties(uint32_t *pPropertyCount, VkExtensionProperties *pProperties) {
    if (pPropertyCount) {
        *pPropertyCount = 0;
    }

    return VK_SUCCESS;
}

VkResult VKAPI_PTR Hook_vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) {
    // Add crash handler for debugging
#ifndef NDEBUG
    SetDebugCrashHandler();
#endif

    auto chainInfo = static_cast<VkLayerInstanceCreateInfo *>(const_cast<void*>(pCreateInfo->pNext));

    // Attempt to find link info
    while (chainInfo && !(chainInfo->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO && chainInfo->function == VK_LAYER_LINK_INFO)) {
        chainInfo = (VkLayerInstanceCreateInfo *) chainInfo->pNext;
    }

    // ...
    if (!chainInfo) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Fetch previous addresses
    PFN_vkGetInstanceProcAddr getInstanceProcAddr = chainInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;

    // Advance layer
    chainInfo->u.pLayerInfo = chainInfo->u.pLayerInfo->pNext;

    // Pass down the chain
    VkResult result = reinterpret_cast<PFN_vkCreateInstance>(getInstanceProcAddr(nullptr, "vkCreateInstance"))(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create dispatch table
    auto table = InstanceDispatchTable::Add(GetInternalTable(*pInstance), new InstanceDispatchTable{});

    // Populate the table
    table->Populate(*pInstance, getInstanceProcAddr);

    // Find optional create info
    if (auto createInfo = FindStructureType<VkGPUOpenGPUReshapeCreateInfo>(pCreateInfo, VK_STRUCTURE_TYPE_GPUOPEN_GPURESHAPE_CREATE_INFO)) {
        // Environment is pre-created at this point
        table->registry.SetParent(createInfo->registry);
    } else {
        // Setup info
        Backend::EnvironmentInfo environmentInfo;
        environmentInfo.device.applicationName = pCreateInfo->pApplicationInfo ? pCreateInfo->pApplicationInfo->pApplicationName : "Unknown";
        environmentInfo.device.apiName = "Vulkan";

        // Initialize the standard environment
        table->environment.Install(environmentInfo);

        // Reparent
        table->registry.SetParent(table->environment.GetRegistry());
    }

    // Get common components
    table->bridge = table->registry.Get<IBridge>();

    // Setup the default allocators
    table->allocators = table->registry.GetAllocators();

    // Create creation deep copy
    table->createInfo.DeepCopy(table->allocators, *pCreateInfo, false);

    // Create application info deep copy
    if (pCreateInfo->pApplicationInfo) {
        table->applicationInfo.DeepCopy(table->allocators, *pCreateInfo->pApplicationInfo);
    }

    // Install shader compiler
#if SHADER_COMPILER_DEBUG
    auto shaderDebug = table->registry.AddNew<ShaderCompilerDebug>(table);
    ENSURE(shaderDebug->Install(), "Failed to install shader debug");
#endif

    // Diagnostic
    table->logBuffer.Add("Vulkan", LogSeverity::Info, "Instance created");

    // OK
    return VK_SUCCESS;
}

void VKAPI_PTR Hook_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    // Get table
    auto table = InstanceDispatchTable::Get(GetInternalTable(instance));

    // Copy destroy
    PFN_vkDestroyInstance next_vkDestroyInstance = table->next_vkDestroyInstance;

    // Release table
    //  ? Done before instance destruction for references
    delete table;

    // Pass down callchain
    next_vkDestroyInstance(instance, pAllocator);
}

void BridgeInstanceSyncPoint(InstanceDispatchTable *table) {
    // Commit bridge
    table->bridge->Commit();
}
