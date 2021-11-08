#include <Backends/Vulkan/Instance.h>
#include <Backends/Vulkan/Layer.h>
#include <Backends/Vulkan/InstanceDispatchTable.h>
#include <Backends/Vulkan/Shader/ShaderCompiler.h>

// Common
#include <Common/Dispatcher.h>
#include <Common/Registry.h>

// Vulkan
#include <vulkan/vk_layer.h>

// Std
#include <cstring>

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
    auto chainInfo = static_cast<const VkLayerInstanceCreateInfo *>(pCreateInfo->pNext);

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
    // TODO: ... there has to be a better way
    const_cast<VkLayerInstanceCreateInfo *>(chainInfo)->u.pLayerInfo = chainInfo->u.pLayerInfo->pNext;

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
        table->registry = createInfo->registry;
    } else {
        ASSERT(false, "Not supported... for now!");
    }

    // Setup the default allocators
    table->allocators = table->registry->GetAllocators();

    // Create the dispatcher
    table->registry->Add(new (table->allocators) Dispatcher());

    // Create the shader compiler
    auto shaderCompiler = table->registry->Add(new (table->allocators) ShaderCompiler());
    shaderCompiler->Initialize();

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
