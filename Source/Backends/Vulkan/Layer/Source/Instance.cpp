#include <Backends/Vulkan/Instance.h>
#include <Backends/Vulkan/Layer.h>
#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>

// Common
#include "Common/Dispatcher/Dispatcher.h"
#include <Common/Registry.h>

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

// Debugging
#ifdef _WIN32
#   ifndef NDEBUG
#       define WIN32_EXCEPTION_HANDLER
#       include <iostream>
#       include <Windows.h>
#   endif
#endif

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

#ifdef WIN32_EXCEPTION_HANDLER
LONG WINAPI TopLevelExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
    // Create console
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);

    // Notify user
    std::cerr << "Layer crashed, waiting for debugger... "  << std::flush;

    // Wait for debugger
    while (!IsDebuggerPresent()) {
        Sleep(100);
    }

    // Notify user
    std::cerr << "Attached."  << std::endl;

    // Break!
    DebugBreak();
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

VkResult VKAPI_PTR Hook_vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) {
#ifdef WIN32_EXCEPTION_HANDLER
    SetUnhandledExceptionFilter(TopLevelExceptionHandler);
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
        table->registry = createInfo->registry;
    } else {
        // Initialize the standard environment
        table->environment.Install(Backend::EnvironmentInfo());
        table->registry = table->environment.GetRegistry();
    }

    // Get common components
    table->bridge = table->registry->Get<IBridge>();

    // Setup the default allocators
    table->allocators = table->registry->GetAllocators();

    // Diagnostic
    table->logBuffer.Add("Vulkan", "Instance created");

    // OK
    return VK_SUCCESS;
}

void VKAPI_PTR Hook_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    // Get table
    auto table = InstanceDispatchTable::Get(GetInternalTable(instance));

    // Pass down callchain
    table->next_vkDestroyInstance(instance, pAllocator);

    // Release table
    delete table;
}

void BridgeSyncPoint(InstanceDispatchTable *table) {
    // Commit all logging to bridge
    table->logBuffer.Commit(table->bridge);

    // Commit bridge
    table->bridge->Commit();
}
