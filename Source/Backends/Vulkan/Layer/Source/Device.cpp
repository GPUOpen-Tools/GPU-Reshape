#include <Backends/Vulkan/Device.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>
#include <Backends/Vulkan/CommandBuffer.h>
#include <Backends/Vulkan/Queue.h>
#include <Backends/Vulkan/Controllers/InstrumentationController.h>
#include <Backends/Vulkan/Compiler/ShaderCompiler.h>
#include <Backends/Vulkan/Allocation/DeviceAllocator.h>
#include <Backends/Vulkan/Export/ShaderExportDescriptorAllocator.h>
#include <Backends/Vulkan/Export/ShaderExportStreamAllocator.h>
#include <Backends/Vulkan/Export/ShaderExportStreamer.h>
#include <Backends/Vulkan/Symbolizer/ShaderSGUIDHost.h>
#include <Backends/Vulkan/Export/ShaderExportHost.h>
#include <Backends/Vulkan/Compiler/ShaderCompiler.h>
#include <Backends/Vulkan/Compiler/PipelineCompiler.h>

// Common
#include <Common/Registry.h>

// Backend
#include <Backend/EnvironmentInfo.h>
#include <Backend/IFeatureHost.h>
#include <Backend/IFeature.h>
#include <Backend/IL/Format.h>
#include <Backend/IL/TextureDimension.h>

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

static void PoolAndInstallFeatures(DeviceDispatchTable* table) {
    // Get the feature host
    auto* host = table->registry->Get<IFeatureHost>();
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

VkResult VKAPI_PTR Hook_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    auto chainInfo = static_cast<VkLayerDeviceCreateInfo *>(const_cast<void*>(pCreateInfo->pNext));

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

    // Advance layer
    chainInfo->u.pLayerInfo = chainInfo->u.pLayerInfo->pNext;

    // Create dispatch table
    auto table = new DeviceDispatchTable{};

    // Inherit shared utilities from the instance
    table->parent     = instanceTable;
    table->allocators = instanceTable->allocators;
    table->registry   = instanceTable->registry;

    // Get the device properties
    table->parent->next_vkGetPhysicalDeviceProperties(physicalDevice, &table->physicalDeviceProperties);

    // Feature chain
    table->physicalDeviceFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    table->physicalDeviceFeatures.pNext = &table->physicalDeviceDescriptorIndexingFeatures;
    table->physicalDeviceDescriptorIndexingFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES};

    // Get the device features
    table->parent->next_vkGetPhysicalDeviceFeatures2(physicalDevice, &table->physicalDeviceFeatures);

    VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES};
    indexingFeatures.pNext = const_cast<void*>(pCreateInfo->pNext);
    indexingFeatures.descriptorBindingStorageTexelBufferUpdateAfterBind = true;

    VkDeviceCreateInfo createInfo = *pCreateInfo;
    createInfo.pNext = &indexingFeatures;

    // Pass down the chain
    VkResult result = reinterpret_cast<PFN_vkCreateDevice>(getInstanceProcAddr(nullptr, "vkCreateDevice"))(physicalDevice, &createInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Populate dispatch table
    DeviceDispatchTable::Add(GetInternalTable(*pDevice), table);
    table->object = *pDevice;
    table->physicalDevice = physicalDevice;

    // Get common components
    table->bridge = table->registry->Get<IBridge>();

    // Populate the table
    table->Populate(getInstanceProcAddr, getDeviceProcAddr);

    // Create the shared allocator
    auto* deviceAllocator = table->registry->AddNew<DeviceAllocator>();
    deviceAllocator->Install(table);

    // Install the shader export host
    table->registry->AddNew<ShaderExportHost>();

    // Install the shader sguid host
    auto* sguidHost = table->registry->AddNew<ShaderSGUIDHost>(table);
    ENSURE(sguidHost->Install(), "Failed to install shader sguid host");

    // Install all features
    PoolAndInstallFeatures(table);

    // Create the proxies / associations between the backend vulkan commands and the features
    CreateDeviceCommandProxies(table);

    // Install the stream allocator
    auto shaderExportStreamAllocator = table->registry->AddNew<ShaderExportStreamAllocator>(table);
    ENSURE(shaderExportStreamAllocator->Install(), "Failed to install stream allocator");

    // Install the stream descriptor allocator
    table->exportDescriptorAllocator = table->registry->AddNew<ShaderExportDescriptorAllocator>(table);
    ENSURE(table->exportDescriptorAllocator->Install(), "Failed to install stream descriptor allocator");

    // Install the streamer
    table->exportStreamer = table->registry->AddNew<ShaderExportStreamer>(table);
    ENSURE(table->exportStreamer->Install(), "Failed to install export streamer allocator");

    // Install the shader compiler
    auto shaderCompiler = table->registry->AddNew<ShaderCompiler>();
    ENSURE(shaderCompiler->Install(), "Failed to install shader compiler");

    // Install the pipeline compiler
    auto pipelineCompiler = table->registry->AddNew<PipelineCompiler>();
    ENSURE(pipelineCompiler->Install(), "Failed to install shader compiler");

    // Install the instrumentation controller
    table->instrumentationController = table->registry->New<InstrumentationController>(table);
    ENSURE(table->instrumentationController->Install(), "Failed to install shader compiler");

    // Create queue states
    for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
        const VkDeviceQueueCreateInfo& info = pCreateInfo->pQueueCreateInfos[i];

        // All queues
        for (uint32_t queueIndex = 0; queueIndex < info.queueCount; queueIndex++) {
            VkQueue queue;
            table->next_vkGetDeviceQueue(table->object, info.queueFamilyIndex, queueIndex, &queue);

            // Create the state
            CreateQueueState(table, queue, info.queueFamilyIndex);
        }
    }

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
