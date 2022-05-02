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
#include <Backends/Vulkan/States/QueueState.h>

// Common
#include <Common/Registry.h>
#include <Common/IComponentTemplate.h>

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

VkResult VKAPI_PTR Hook_vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkLayerProperties *pProperties) {
    auto table = InstanceDispatchTable::Get(GetInternalTable(physicalDevice));

    // Enumerate base layer count
    VkResult result = table->next_vkEnumerateDeviceLayerProperties(physicalDevice, pPropertyCount, nullptr);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Local layer
    *pPropertyCount += 1;

    // Filling?
    if (pProperties) {
        // Enumerate base layers
        result = table->next_vkEnumerateDeviceLayerProperties(physicalDevice, pPropertyCount, pProperties);
        if (result != VK_SUCCESS) {
            return result;
        }

        // Fill local layer
        VkLayerProperties& localProperty = pProperties[*pPropertyCount - 1];
        strcpy_s(localProperty.layerName, VK_GPUOPEN_GPUVALIDATION_LAYER_NAME);
        strcpy_s(localProperty.description, "");
        localProperty.implementationVersion = 1;
        localProperty.specVersion = VK_API_VERSION_1_0;
    }

    // OK
    return VK_SUCCESS;
}

VkResult VKAPI_PTR Hook_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties) {
    auto table = InstanceDispatchTable::Get(GetInternalTable(physicalDevice));

    // Local layer?
    if (pLayerName && !std::strcmp(pLayerName, VK_GPUOPEN_GPUVALIDATION_LAYER_NAME)) {
        *pPropertyCount = 0;
        return VK_SUCCESS;
    }

    return table->next_vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount, pProperties);
}

static bool PoolAndInstallFeatures(DeviceDispatchTable* table) {
    // Get the feature host
    auto host = table->registry.Get<IFeatureHost>();
    if (!host) {
        return false;
    }

    // All templates
    std::vector<ComRef<IComponentTemplate>> templates;

    // Pool feature count
    uint32_t featureCount;
    host->Enumerate(&featureCount, nullptr);

    // Pool features
    templates.resize(featureCount);
    host->Enumerate(&featureCount, templates.data());

    // Install features
    for (const ComRef<IComponentTemplate>& _template : templates) {
        // Instantiate feature to this registry
        auto feature = Cast<IFeature>(_template->Instantiate(&table->registry));

        // Try to install feature
        if (!feature->Install()) {
            return false;
        }

        table->features.push_back(feature);
    }

    return true;
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

    // Initialize registry
    table->registry.SetParent(&instanceTable->registry);

    // Get the device properties
    table->parent->next_vkGetPhysicalDeviceProperties(physicalDevice, &table->physicalDeviceProperties);

    // Feature chain
    table->physicalDeviceFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    table->physicalDeviceFeatures.pNext = &table->physicalDeviceDescriptorIndexingFeatures;
    table->physicalDeviceDescriptorIndexingFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES};

    // Get the device features
    table->parent->next_vkGetPhysicalDeviceFeatures2(physicalDevice, &table->physicalDeviceFeatures);

    // Create a deep copy
    table->createInfo.DeepCopy(table->allocators, *pCreateInfo);

    // Copy layers and extensions
    std::vector<const char*> layers(table->createInfo->ppEnabledLayerNames, table->createInfo->ppEnabledLayerNames + table->createInfo->enabledLayerCount);
    std::vector<const char*> extensions(table->createInfo->ppEnabledExtensionNames, table->createInfo->ppEnabledExtensionNames + table->createInfo->enabledExtensionCount);

    // Add descriptor indexing extension
    extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

    // Find existing indexing features or allocate a new one
    auto* indexingFeatures = FindStructureTypeMutableUnsafe<VkPhysicalDeviceDescriptorIndexingFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES>(table->createInfo->pNext);
    if (!indexingFeatures) {
        indexingFeatures = new (ALLOCA(VkPhysicalDeviceDescriptorIndexingFeatures)) VkPhysicalDeviceDescriptorIndexingFeatures{};
        indexingFeatures->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
        PrependExtensionUnsafe(&table->createInfo.createInfo, indexingFeatures);
    }

    // Enable update after bind
    indexingFeatures->descriptorBindingStorageTexelBufferUpdateAfterBind = true;

    // Set new layers and extensions
    table->createInfo->ppEnabledLayerNames = layers.data();
    table->createInfo->enabledLayerCount = static_cast<uint32_t>(layers.size());
    table->createInfo->ppEnabledExtensionNames = extensions.data();
    table->createInfo->enabledExtensionCount = static_cast<uint32_t>(extensions.size());

    // Pass down the chain
    VkResult result = reinterpret_cast<PFN_vkCreateDevice>(getInstanceProcAddr(nullptr, "vkCreateDevice"))(physicalDevice, &table->createInfo.createInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Populate dispatch table
    DeviceDispatchTable::Add(GetInternalTable(*pDevice), table);
    table->object = *pDevice;
    table->physicalDevice = physicalDevice;

    // Get common components
    table->bridge = table->registry.Get<IBridge>();

    // Populate the table
    table->Populate(getInstanceProcAddr, getDeviceProcAddr);

    // Create the shared allocator
    auto deviceAllocator = table->registry.AddNew<DeviceAllocator>();
    deviceAllocator->Install(table);

    // Install the shader export host
    table->registry.AddNew<ShaderExportHost>();

    // Install the shader sguid host
    table->sguidHost = table->registry.AddNew<ShaderSGUIDHost>(table);
    ENSURE(table->sguidHost->Install(), "Failed to install shader sguid host");

    // Install all features
    ENSURE(PoolAndInstallFeatures(table), "Failed to install features");

    // Create the proxies / associations between the backend vulkan commands and the features
    CreateDeviceCommandProxies(table);

    // Install the stream allocator
    auto shaderExportStreamAllocator = table->registry.AddNew<ShaderExportStreamAllocator>(table);
    ENSURE(shaderExportStreamAllocator->Install(), "Failed to install stream allocator");

    // Install the stream descriptor allocator
    table->exportDescriptorAllocator = table->registry.AddNew<ShaderExportDescriptorAllocator>(table);
    ENSURE(table->exportDescriptorAllocator->Install(), "Failed to install stream descriptor allocator");

    // Install the streamer
    table->exportStreamer = table->registry.AddNew<ShaderExportStreamer>(table);
    ENSURE(table->exportStreamer->Install(), "Failed to install export streamer allocator");

    // Install the shader compiler
    auto shaderCompiler = table->registry.AddNew<ShaderCompiler>(table);
    ENSURE(shaderCompiler->Install(), "Failed to install shader compiler");

    // Install the pipeline compiler
    auto pipelineCompiler = table->registry.AddNew<PipelineCompiler>(table);
    ENSURE(pipelineCompiler->Install(), "Failed to install shader compiler");

    // Install the instrumentation controller
    table->instrumentationController = table->registry.New<InstrumentationController>(table);
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

    // Ensure all work is done
    table->next_vkDeviceWaitIdle(device);

    // Process all remaining work
    table->exportStreamer->Process();

    // Manual uninstalls
    table->instrumentationController->Uninstall();

    // Release all features
    table->features.clear();

    // Destroy all queue states
    for (QueueState* queueState : table->states_queue.GetLinear()) {
        destroy(queueState, table->allocators);
    }

    // Copy destroy
    PFN_vkDestroyDevice next_vkDestroyDevice = table->next_vkDestroyDevice;

    // Release table
    //  ? Done before device destruction for references
    delete table;

    // Pass down callchain
    next_vkDestroyDevice(device, pAllocator);
}
