#include <Callbacks.h>
#include <DispatchTables.h>

#include <vulkan/vk_layer.h>
#include <cstring>

VkResult CreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) {
	// Attempt to find link info
	auto chain_info = (VkLayerInstanceCreateInfo*)pCreateInfo->pNext;
	while (chain_info && !(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO && chain_info->function == VK_LAYER_LINK_INFO)) {
		chain_info = (VkLayerInstanceCreateInfo*)chain_info->pNext;
	}

	// ...
	if (!chain_info) {
		return VK_ERROR_INITIALIZATION_FAILED;
	}

    // Fetch previous addresses
	PFN_vkGetInstanceProcAddr getInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;

    // Advance layer
	chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    // Pass down the chain
    VkResult result = reinterpret_cast<PFN_vkCreateInstance>(getInstanceProcAddr(nullptr, "vkCreateInstance"))(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Populate dispatch table
    InstanceDispatchTable table{};
	table.m_Instance = *pInstance;
    table.m_GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) getInstanceProcAddr(*pInstance, "vkGetInstanceProcAddr");
    table.m_DestroyInstance = (PFN_vkDestroyInstance) getInstanceProcAddr(*pInstance, "vkDestroyInstance");
    table.m_EnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties) getInstanceProcAddr(*pInstance, "vkEnumerateDeviceExtensionProperties");
	table.m_GetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)getInstanceProcAddr(*pInstance, "vkGetPhysicalDeviceQueueFamilyProperties");

	// Find this layer
	uint32_t layer_index = 0;
	for (; layer_index < pCreateInfo->enabledLayerCount; layer_index++)
	{
		if (!std::strcmp("VK_LAYER_AVA_gpu_validation", pCreateInfo->ppEnabledLayerNames[layer_index]))
			break;
	}

	// Attempt to find any khronos layer below this layer
	// I.e. will any of our calls be routed?
	table.m_RequiresDispatchTablePatching = false;
	for (++layer_index; layer_index < pCreateInfo->enabledLayerCount; layer_index++)
	{
		table.m_RequiresDispatchTablePatching = (std::strcmp("VK_LAYER_LUNARG_", pCreateInfo->ppEnabledLayerNames[layer_index]) <= 0 || std::strcmp("VK_LAYER_KHRONOS_", pCreateInfo->ppEnabledLayerNames[layer_index]) <= 0);
	}

    InstanceDispatchTable::Add(GetKey(*pInstance), table);

    return VK_SUCCESS;
}

VkResult VKAPI_CALL EnumerateInstanceLayerProperties(uint32_t *pPropertyCount, VkLayerProperties *pProperties) {
    if (pPropertyCount) {
        *pPropertyCount = 1;
    }

    if (pProperties) {
        std::strcpy(pProperties->layerName, "VK_LAYER_AVA_GPU_VALIDATION");
        std::strcpy(pProperties->description, "Validates potentially undefined behaviour on the GPU");
        pProperties->implementationVersion = 1;
        pProperties->specVersion = VK_API_VERSION_1_0;
    }

    return VK_SUCCESS;
}

VK_LAYER_EXPORT VkResult VKAPI_CALL EnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties) {
    if (!pLayerName || std::strcmp(pLayerName, "VK_LAYER_AVA_GPU_VALIDATION") != 0) {
        return VK_ERROR_LAYER_NOT_PRESENT;
    }

    if (pPropertyCount) *pPropertyCount = 0;
    return VK_SUCCESS;
}

void VKAPI_CALL DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    // Remove table
    InstanceDispatchTable table = InstanceDispatchTable::Get(GetKey(instance));
    InstanceDispatchTable::Remove(GetKey(instance));

    // Pass down call chain
    table.m_DestroyInstance(instance, pAllocator);
}
