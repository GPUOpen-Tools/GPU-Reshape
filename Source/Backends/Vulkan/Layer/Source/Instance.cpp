#include <Backends/Vulkan/Instance.h>
#include <Backends/Vulkan/Layer.h>
#include <Backends/Vulkan/InstanceDispatchTable.h>
#include <Backends/Vulkan/Compiler/ShaderCompiler.h>
#include <Backends/Vulkan/Compiler/PipelineCompiler.h>
#include <Backends/Vulkan/Export/ShaderExportHost.h>

// Common
#include <Common/Dispatcher.h>
#include <Common/Registry.h>

// Backend
#include <Backend/EnvironmentInfo.h>
#include <Backend/IFeatureHost.h>
#include <Backend/IFeature.h>

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

static void PoolAndInstallFeatures(InstanceDispatchTable* table) {
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

    // Install features
    for (IFeature* feature : table->features) {
        feature->Install();
    }
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
        // Environment is pre-created at this point
        table->registry = createInfo->registry;
    } else {
        // Initialize the standard environment
        table->environment.Install(Backend::EnvironmentInfo());
        table->registry = table->environment.GetRegistry();
    }

    // Setup the default allocators
    table->allocators = table->registry->GetAllocators();

    // Install the shader export host
    table->registry->AddNew<ShaderExportHost>();

    // Install the shader compiler
    auto shaderCompiler = table->registry->AddNew<ShaderCompiler>();
    shaderCompiler->Install();

    // Install the pipeline compiler
    auto pipelineCompiler = table->registry->AddNew<PipelineCompiler>();
    pipelineCompiler->Install();

    // Install all features
    PoolAndInstallFeatures(table);

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
