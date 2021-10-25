#include <Backends/Vulkan/Device.h>
#include <Backends/Vulkan/DeviceDispatchTable.h>

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

    // Populate the table
    table->Populate(*pDevice, getInstanceProcAddr, getDeviceProcAddr);

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
