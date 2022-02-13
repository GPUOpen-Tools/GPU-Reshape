#include <Backends/Vulkan/Instance.h>
#include <Backends/Vulkan/Layer.h>
#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>

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
        strcpy_s(pProperties->layerName, "VK_GPUOpen_GBV");
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
    if (auto createInfo = FindStructureType<VkGPUOpenGPUValidationCreateInfo>(pCreateInfo, VK_STRUCTURE_TYPE_GPUOPEN_GPUVALIDATION_CREATE_INFO)) {
        // Environment is pre-created at this point
        table->registry.SetParent(createInfo->registry);
    } else {
        // Setup info
        Backend::EnvironmentInfo environmentInfo;
        environmentInfo.applicationName = pCreateInfo->pApplicationInfo ? pCreateInfo->pApplicationInfo->pApplicationName : "Unknown";

        // Initialize the standard environment
        table->environment.Install(environmentInfo);

        // Reparent
        table->registry.SetParent(table->environment.GetRegistry());
    }

    // Get common components
    table->bridge = table->registry.Get<IBridge>();

    // Setup the default allocators
    table->allocators = table->registry.GetAllocators();

    // Diagnostic
    table->logBuffer.Add("Vulkan", "Instance created");

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

void BridgeSyncPoint(InstanceDispatchTable *table) {
    // Commit all logging to bridge
    table->logBuffer.Commit(table->bridge.GetUnsafe());

    // Commit bridge
    table->bridge->Commit();
}
