#include <Backends/Vulkan/Device.h>
#include <Backends/Vulkan/DeviceDispatchTable.h>
#include <Backends/Vulkan/InstanceDispatchTable.h>
#include <Backends/Vulkan/CommandBuffer.h>
#include <Backends/Vulkan/Controllers/InstrumentationController.h>
#include <Backends/Vulkan/Compiler/ShaderCompiler.h>

// Common
#include <Common/Registry.h>

// Backend
#include <Backend/IFeatureHost.h>

// Bridge
#include <Bridge/IBridge.h>

// Vulkan
#include <vulkan/vk_layer.h>

// Std
#include <cstring>

VkResult VKAPI_PTR Hook_vkEnumerateDeviceLayerProperties(uint32_t *pPropertyCount, VkLayerProperties *pProperties) {
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

VkResult VKAPI_PTR Hook_vkEnumerateDeviceExtensionProperties(uint32_t *pPropertyCount, VkExtensionProperties *pProperties) {
    if (pPropertyCount) {
        *pPropertyCount = 0;
    }

    return VK_SUCCESS;
}

void PoolDeviceFeatures(DeviceDispatchTable* table) {
    // Get the feature host
    IFeatureHost* host = table->registry->Get<IFeatureHost>();
    if (!host) {
        return;
    }

    // Pool feature count
    uint32_t featureCount;
    host->Enumerate(&featureCount, nullptr);

    // Pool features
    table->features.resize(featureCount);
    host->Enumerate(&featureCount, table->features.data());
}

VkResult VKAPI_PTR Hook_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    auto chainInfo = static_cast<const VkLayerDeviceCreateInfo *>(pCreateInfo->pNext);

    // Attempt to find link info
    while (chainInfo && !(chainInfo->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO && chainInfo->function == VK_LAYER_LINK_INFO)) {
        chainInfo = (VkLayerDeviceCreateInfo *) chainInfo->pNext;
    }

    // ...
    if (!chainInfo) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Get the instance table
    auto instanceTable = InstanceDispatchTable::Get(GetInternalTable(physicalDevice));

    // Fetch previous addresses
    PFN_vkGetInstanceProcAddr getInstanceProcAddr = chainInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr getDeviceProcAddr = chainInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr;

    // Pass down the chain
    VkResult result = reinterpret_cast<PFN_vkCreateDevice>(getInstanceProcAddr(nullptr, "vkCreateDevice"))(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create dispatch table
    auto table = DeviceDispatchTable::Add(GetInternalTable(*pDevice), new DeviceDispatchTable{});

    // Inherit shared utilities from the instance
    table->allocators = instanceTable->allocators;
    table->registry   = instanceTable->registry;

    // Get common components
    table->bridge = table->registry->Get<IBridge>();

    // Populate the table
    table->Populate(*pDevice, getInstanceProcAddr, getDeviceProcAddr);

    // Pool all features
    PoolDeviceFeatures(table);

    // Create the proxies / associations between the backend vulkan commands and the features
    CreateDeviceCommandProxies(table);

    // Create the instrumentation controller
    table->instrumentationController = new (table->allocators) InstrumentationController(table->registry, table);
    table->bridge->Register(table->instrumentationController);

    // OK
    return VK_SUCCESS;
}

void VKAPI_PTR Hook_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    // Get table
    auto table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    table->next_vkDestroyDevice(device, pAllocator);

    // Release table
    delete table;
}
