// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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

#include <Backends/Vulkan/Device.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>
#include <Backends/Vulkan/Instance.h>
#include <Backends/Vulkan/CommandBuffer.h>
#include <Backends/Vulkan/Queue.h>
#include <Backends/Vulkan/Controllers/InstrumentationController.h>
#include <Backends/Vulkan/Controllers/FeatureController.h>
#include <Backends/Vulkan/Controllers/MetadataController.h>
#include <Backends/Vulkan/Controllers/VersioningController.h>
#include <Backends/Vulkan/Compiler/ShaderCompiler.h>
#include <Backends/Vulkan/Allocation/DeviceAllocator.h>
#include <Backends/Vulkan/Export/ShaderExportDescriptorAllocator.h>
#include <Backends/Vulkan/Export/ShaderExportStreamAllocator.h>
#include <Backends/Vulkan/Export/ShaderExportStreamer.h>
#include <Backends/Vulkan/Symbolizer/ShaderSGUIDHost.h>
#include <Backends/Vulkan/ShaderData/ShaderDataHost.h>
#include <Backends/Vulkan/Export/ShaderExportHost.h>
#include <Backends/Vulkan/Compiler/ShaderCompiler.h>
#include <Backends/Vulkan/Compiler/PipelineCompiler.h>
#include <Backends/Vulkan/States/QueueState.h>
#include <Backends/Vulkan/Resource/PhysicalResourceMappingTable.h>
#include <Backends/Vulkan/ShaderProgram/ShaderProgramHost.h>
#include <Backends/Vulkan/Scheduler/Scheduler.h>
#include <Backends/Vulkan/QueueInfoWriter.h>

// Common
#include <Common/Registry.h>
#include <Common/IComponentTemplate.h>

// Backend
#include <Backend/EnvironmentInfo.h>
#include <Backend/StartupEnvironment.h>
#include <Backend/StartupContainer.h>
#include <Backend/IFeatureHost.h>
#include <Backend/IFeature.h>
#include <Backend/IL/Format.h>
#include <Backend/IL/TextureDimension.h>

// Message
#include <Message/OrderedMessageStorage.h>

// Bridge
#include <Bridge/IBridge.h>

// Vulkan
#include <vulkan/vk_layer.h>

// Std
#include <cstring>

static void ApplyStartupEnvironment(DeviceDispatchTable* table) {
    // Get container
    auto container = table->registry.Get<Backend::StartupContainer>();

    // Empty?
    if (container->stream.IsEmpty()) {
        return;
    }
    
    // Commit initial stream
    table->bridge->GetInput()->AddStream(container->stream);
    table->bridge->Commit();
}

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
        strcpy_s(localProperty.layerName, VK_GPUOPEN_GPURESHAPE_LAYER_NAME);
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
    if (pLayerName && !std::strcmp(pLayerName, VK_GPUOPEN_GPURESHAPE_LAYER_NAME)) {
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

    // Pool feature count
    uint32_t featureCount;
    host->Install(&featureCount, nullptr, nullptr);

    // Pool features
    table->features.resize(featureCount);
    return host->Install(&featureCount, table->features.data(), &table->registry);
}

static void CreateEventRemappingTable(DeviceDispatchTable* table) {
    // All data
    std::vector<ShaderDataInfo> data;

    // Pool feature count
    uint32_t dataCount;
    table->dataHost->Enumerate(&dataCount, nullptr, ShaderDataType::Event);

    // Pool features
    data.resize(dataCount);
    table->dataHost->Enumerate(&dataCount, data.data(), ShaderDataType::Event);

    // Current offset
    uint32_t offset = 0;

    // Populate table
    for (const ShaderDataInfo& info : data) {
        if (info.id >= table->eventRemappingTable.Size()) {
            table->eventRemappingTable.Resize(info.id + 1);
        }

        table->eventRemappingTable[info.id] = offset;

        // Next dword
        offset += sizeof(uint32_t);
    }
}

static Backend::EnvironmentDeviceInfo GetEnvironmentDeviceInfo(DeviceDispatchTable* device) {
    Backend::EnvironmentDeviceInfo info;
    info.applicationName = device->parent->createInfo->pApplicationInfo ? device->parent->createInfo->pApplicationInfo->pApplicationName : nullptr;
    info.apiName = "Vulkan";
    info.deviceUID = device->uid;

    // Ensure there's a valid name
    if (!info.applicationName) {
        info.applicationName = "Unknown";
    }

    // Accumulate objects
    info.deviceObjects += static_cast<uint32_t>(device->states_buffer.GetCount());
    info.deviceObjects += static_cast<uint32_t>(device->states_image.GetCount());

    // OK
    return info;
}

static void DeviceSyncPoint(DeviceDispatchTable *table) {
    // Commit bridge data
    BridgeDeviceSyncPoint(table, nullptr);
}

template<typename T>
static void EnableDescriptorFeatureSet(T* features) {
    features->descriptorBindingStorageTexelBufferUpdateAfterBind = true;
    features->descriptorBindingUniformTexelBufferUpdateAfterBind = true;
}

template<typename T>
static void EnableFeatureSet(T* features) {
    features->vertexPipelineStoresAndAtomics = true;
    features->fragmentStoresAndAtomics = true;
    features->sparseBinding = true;
    features->sparseResidencyBuffer = true;
}

static Backend::VendorType GetVendor(uint32_t vendorID) {
    // Assume from vendor id
    switch (vendorID) {
        default: {
            return Backend::VendorType::Unknown;
        }
        case 0x1002:
        case 0x1022: {
            return Backend::VendorType::AMD;
        }
        case 0x10DE: {
            return Backend::VendorType::Nvidia;
        }
        case 0x163C:
        case 0x8086:
        case 0x8087: {
            return Backend::VendorType::Intel;
        }
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
    table->uid = VulkanGPUReshapeProcessInfo.deviceUID++;

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
    table->physicalDeviceDescriptorIndexingFeatures.pNext = &table->physicalDeviceRobustness2Features;
    table->physicalDeviceRobustness2Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT};

    // Get the device features
    table->parent->next_vkGetPhysicalDeviceFeatures2(physicalDevice, &table->physicalDeviceFeatures);

    // Try to get the vendor
    table->vendor = GetVendor(table->physicalDeviceProperties.vendorID);

    // Create a deep copy
    table->createInfo.DeepCopy(table->allocators, *pCreateInfo);

    // Copy layers and extensions
    table->enabledLayers.insert(table->enabledLayers.end(), table->createInfo->ppEnabledLayerNames, table->createInfo->ppEnabledLayerNames + table->createInfo->enabledLayerCount);
    table->enabledExtensions.insert(table->enabledExtensions.end(), table->createInfo->ppEnabledExtensionNames, table->createInfo->ppEnabledExtensionNames + table->createInfo->enabledExtensionCount);

    // Add descriptor indexing extension
    table->enabledExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
    
    // Add synchronization extensions
    table->enabledExtensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
    table->enabledExtensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

    // Optional feature structures
    auto* features2                = FindStructureTypeMutableUnsafe<VkPhysicalDeviceFeatures2, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2>(table->createInfo->pNext);
    auto* features1_2              = FindStructureTypeMutableUnsafe<VkPhysicalDeviceVulkan12Features, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES>(table->createInfo->pNext);
    auto* features1_3              = FindStructureTypeMutableUnsafe<VkPhysicalDeviceVulkan13Features, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES>(table->createInfo->pNext);
    auto* indexingFeatures         = FindStructureTypeMutableUnsafe<VkPhysicalDeviceDescriptorIndexingFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES>(table->createInfo->pNext);
    auto* timelineFeatures         = FindStructureTypeMutableUnsafe<VkPhysicalDeviceTimelineSemaphoreFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES>(table->createInfo->pNext);
    auto* synchronization2Features = FindStructureTypeMutableUnsafe<VkPhysicalDeviceSynchronization2Features, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES>(table->createInfo->pNext);

    // Try enabling descriptor features
    VkPhysicalDeviceDescriptorIndexingFeatures indexingFeaturesFallback{};
    if (features1_2) {
        EnableDescriptorFeatureSet(features1_2);
    } else if (indexingFeatures) {
        EnableDescriptorFeatureSet(indexingFeatures);
    } else {
        EnableDescriptorFeatureSet(&indexingFeaturesFallback);
        indexingFeaturesFallback.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
        PrependExtensionUnsafe(&table->createInfo.createInfo, &indexingFeaturesFallback);
    }

    // Try enabling synchronization2
    VkPhysicalDeviceSynchronization2Features synchronization2Fallback{};
    if (features1_3) {
        features1_3->synchronization2 = true;
    } else if (synchronization2Features) {
        synchronization2Features->synchronization2 = true;
    } else {
        synchronization2Fallback.synchronization2 = true;
        synchronization2Fallback.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
        PrependExtensionUnsafe(&table->createInfo.createInfo, &synchronization2Fallback);
    }

    // Try enabling timeline features
    VkPhysicalDeviceTimelineSemaphoreFeatures timelineFallback{};
    if (features1_2) {
        features1_2->timelineSemaphore = true;
    } else if (timelineFeatures) {
        timelineFeatures->timelineSemaphore = true;
    } else {
        timelineFallback.timelineSemaphore = true;
        timelineFallback.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
        PrependExtensionUnsafe(&table->createInfo.createInfo, &timelineFallback);
    }

    // Try enabling base features
    VkPhysicalDeviceFeatures featuresFallback{};
    if (features2) {
        EnableFeatureSet(&features2->features);
    } else {
        if (table->createInfo->pEnabledFeatures) {
            featuresFallback = *table->createInfo->pEnabledFeatures;
        }

        EnableFeatureSet(&featuresFallback);
        table->createInfo->pEnabledFeatures = &featuresFallback;
    }

    // Set new layers and extensions
    table->createInfo->ppEnabledLayerNames = table->enabledLayers.data();
    table->createInfo->enabledLayerCount = static_cast<uint32_t>(table->enabledLayers.size());
    table->createInfo->ppEnabledExtensionNames = table->enabledExtensions.data();
    table->createInfo->enabledExtensionCount = static_cast<uint32_t>(table->enabledExtensions.size());

    // Get the number of families
    uint32_t queueFamilyPropertyCount;
    table->parent->next_vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

    // Get all families
    table->queueFamilyProperties.resize(queueFamilyPropertyCount);
    table->parent->next_vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, table->queueFamilyProperties.data());

    // Create queue writer
    QueueInfoWriter queueWriter(table);

    // Request exclusive queues of given type
    table->preferredExclusiveGraphicsQueue = queueWriter.RequestExclusiveQueueOfType(VK_QUEUE_GRAPHICS_BIT);
    table->preferredExclusiveComputeQueue  = queueWriter.RequestExclusiveQueueOfType(VK_QUEUE_COMPUTE_BIT);
    table->preferredExclusiveTransferQueue = queueWriter.RequestExclusiveQueueOfType(VK_QUEUE_TRANSFER_BIT);

    // Set new queues
    queueWriter.Assign();
    
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

    // Install the stream descriptor allocator
    table->dataHost = table->registry.AddNew<ShaderDataHost>(table);
    ENSURE(table->dataHost->Install(), "Failed to install data host");

    // Create the program host
    table->shaderProgramHost = table->registry.AddNew<ShaderProgramHost>(table);
    ENSURE(table->shaderProgramHost->Install(), "Failed to install shader program host");

    // Install the scheduler
    table->scheduler = table->registry.AddNew<Scheduler>(table);
    ENSURE(table->scheduler->Install(), "Failed to install scheduler");

    // Install all features
    ENSURE(PoolAndInstallFeatures(table), "Failed to install features");

    // Create remapping table
    CreateEventRemappingTable(table);

    // Create constant remapping table
    table->constantRemappingTable = table->dataHost->CreateConstantMappingTable();

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
    ENSURE(pipelineCompiler->Install(), "Failed to install pipeline compiler");

    // Install the instrumentation controller
    table->instrumentationController = table->registry.New<InstrumentationController>(table);
    ENSURE(table->instrumentationController->Install(), "Failed to install instrumentation controller");

    // Install the feature controller
    table->featureController = table->registry.AddNew<FeatureController>(table);
    ENSURE(table->featureController->Install(), "Failed to install feature controller");

    // Install the metadata controller
    table->metadataController = table->registry.New<MetadataController>(table);
    ENSURE(table->metadataController->Install(), "Failed to install metadata controller");

    // Install the versioning controller
    table->versioningController = table->registry.New<VersioningController>(table);
    ENSURE(table->versioningController->Install(), "Failed to install versioning controller");

    // Create the physical resource table
    table->prmTable = table->registry.New<PhysicalResourceMappingTable>(table);
    ENSURE(table->prmTable->Install(), "Failed to install PRM table");

    // Install all user programs, done after feature creation for data pooling
    ENSURE(table->shaderProgramHost->InstallPrograms(), "Failed to install shader program host programs");

    // Create queue states
    for (uint32_t i = 0; i < table->createInfo->queueCreateInfoCount; i++) {
        const VkDeviceQueueCreateInfo& info = table->createInfo->pQueueCreateInfos[i];

        // All queues
        for (uint32_t queueIndex = 0; queueIndex < info.queueCount; queueIndex++) {
            VkQueue queue;
            table->next_vkGetDeviceQueue(table->object, info.queueFamilyIndex, queueIndex, &queue);

            // Create the state
            CreateQueueState(table, queue, info.queueFamilyIndex);
        }
    }

    // Apply environment
    ApplyStartupEnvironment(table);

    // Finally, post-install all features for late work
    // This must be done after all dependent states are initialized
    for (const ComRef<IFeature>& feature : table->features) {
        ENSURE(feature->PostInstall(), "Failed to post-install feature");
    }

    // Start sync thread
    table->syncPointActionThread.Start(std::bind(DeviceSyncPoint, table));

    // OK
    return VK_SUCCESS;
}

void VKAPI_PTR Hook_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    // Get table
    auto table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Wait for all pending instrumentation
    table->instrumentationController->WaitForCompletion();

    // Ensure all work is done
    table->next_vkDeviceWaitIdle(device);

    // Stop the sync point thread
    table->syncPointActionThread.Stop();

    // Process all remaining work
    table->exportStreamer->Process();

    // Wait for all pending submissions
    table->scheduler->WaitForPending();

    // Manual uninstalls
    table->versioningController->Uninstall();
    table->metadataController->Uninstall();
    table->instrumentationController->Uninstall();
    table->featureController->Uninstall();

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

void BridgeDeviceSyncPoint(DeviceDispatchTable *table, ShaderExportQueueState* queueState) {
    // Commit all logging to bridge
    table->parent->logBuffer.Commit(table->bridge.GetUnsafe());
    
    // Commit controllers
    table->featureController->Commit();
    table->instrumentationController->Commit();
    table->metadataController->Commit();
    table->versioningController->Commit();

    // Inform the streamer of the sync point
    if (queueState) {
        table->exportStreamer->Process(queueState);
    } else {
        table->exportStreamer->Process();
    }

    // Update the environment?
    if (table->environmentUpdateAction.Step()) {
        table->parent->environment.Update(GetEnvironmentDeviceInfo(table));
    }

    // Commit instance
    BridgeInstanceSyncPoint(table->parent);
}
