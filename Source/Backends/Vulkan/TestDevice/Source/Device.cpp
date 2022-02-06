#include <Test/Device/Vulkan/Device.h>
#include <Test/Device/Catch2.h>

// Vulkan
#include <Backends/Vulkan/Layer.h>
#include <Backends/Vulkan/Translation.h>

// Common
#include <Common/Path.h>

using namespace Test;
using namespace Test::Vulkan;

void Device::Install(const DeviceInfo &creationInfo) {
    deviceInfo = creationInfo;

    // Get all instance extensions
    EnumerateInstanceExtensions();

    // Create with enabled extension set
    CreateInstance();

    // Create optional messenger
    if (creationInfo.enableValidation) {
        CreateDebugMessenger();
    }

    // Get all device extensions
    EnumerateDeviceExtensions();

    // Create with enabled extension set
    CreateDevice();

    // Create allocators
    CreateAllocator();

    // Create shared pools
    CreateSharedDescriptorPool();
    CreateSharedQueuePools();
}

void Device::EnumerateInstanceExtensions() {
    // Redirect layer path
    _putenv_s("VK_LAYER_PATH", CurrentExecutableDirectory().string().c_str());

    // Get number of instance layers
    uint32_t instanceLayerCount;
    REQUIRE(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr) == VK_SUCCESS);

    // Enumerate instance layers
    instanceLayers.resize(instanceLayerCount);
    REQUIRE(vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.data()) == VK_SUCCESS);

    // Get number of instance extensions
    uint32_t instanceExtensionCount;
    REQUIRE(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr) == VK_SUCCESS);

    // Enumerate instance extensions
    instanceExtensions.resize(instanceExtensionCount);
    REQUIRE(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensions.data()) == VK_SUCCESS);
}

bool Device::SupportsInstanceLayer(const char *name) const {
    return std::any_of(instanceLayers.begin(), instanceLayers.end(), [&](const VkLayerProperties &properties) {
        return std::strcmp(properties.layerName, name) == 0;
    });
}

bool Device::SupportsInstanceExtension(const char *name) const {
    return std::any_of(instanceExtensions.begin(), instanceExtensions.end(), [&](const VkExtensionProperties &properties) {
        return std::strcmp(properties.extensionName, name) == 0;
    });
}

bool Device::SupportsDeviceExtension(const char *name) const {
    return std::any_of(deviceExtensions.begin(), deviceExtensions.end(), [&](const VkExtensionProperties &properties) {
        return std::strcmp(properties.extensionName, name) == 0;
    });
}

void Device::CreateInstance() {
    // All requested extensions
    std::vector<const char *> enabledLayers = {};
    std::vector<const char *> enabledExtensions = {};

    // Must support the gpu validation layer
    enabledLayers.push_back(VK_GPUOPEN_GPUVALIDATION_LAYER_NAME);

    // With validation?
    if (deviceInfo.enableValidation) {
        enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
        enabledExtensions.push_back("VK_EXT_debug_utils");
    }

    // Must support all layers
    for (const char *layer: enabledLayers) {
        REQUIRE_FORMAT(SupportsInstanceLayer(layer), "Missing layer: " << layer);
    }

    // Must support all extensions
    for (const char *extension: enabledExtensions) {
        REQUIRE_FORMAT(SupportsInstanceExtension(extension), "Missing extension: " << extension);
    }

    // General app info
    VkApplicationInfo applicationInfo{};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.apiVersion = VK_API_VERSION_1_2;
    applicationInfo.pApplicationName = "GPUOpen GBV";
    applicationInfo.pEngineName = "GPUOpen GBV";

    // Pass down the environment
    VkGPUOpenGPUValidationCreateInfo gpuOpenInfo{};
    gpuOpenInfo.sType = VK_STRUCTURE_TYPE_GPUOPEN_GPUVALIDATION_CREATE_INFO;
    gpuOpenInfo.registry = registry;

    // Instance info
    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = &gpuOpenInfo;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledExtensionCount = enabledExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
    instanceCreateInfo.enabledLayerCount = enabledLayers.size();
    instanceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
    REQUIRE(vkCreateInstance(&instanceCreateInfo, nullptr, &instance) == VK_SUCCESS);

    // Get the number of physical devices
    uint32_t physicalDeviceCount;
    REQUIRE(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) == VK_SUCCESS);

    // Must have at least one device
    REQUIRE(physicalDeviceCount > 0);

    // Get all physical devices
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    REQUIRE(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()) == VK_SUCCESS);

    // TODO: Make sure it's at least dedicated
    physicalDevice = physicalDevices.at(0);
}

void Device::CreateDebugMessenger() {
    // Validation create info
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
    createInfo.pUserData = nullptr;

    // Validation proc
    auto next_vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    REQUIRE(next_vkCreateDebugUtilsMessengerEXT);

    // Attempt to create the validation messenger
    REQUIRE(next_vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) == VK_SUCCESS);
}

void Device::EnumerateDeviceExtensions() {
    // Get number of device layers
    uint32_t deviceLayerCount;
    REQUIRE(vkEnumerateDeviceLayerProperties(physicalDevice, &deviceLayerCount, nullptr) == VK_SUCCESS);

    // Enumerate device layers
    deviceLayers.resize(deviceLayerCount);
    REQUIRE(vkEnumerateDeviceLayerProperties(physicalDevice, &deviceLayerCount, deviceLayers.data()) == VK_SUCCESS);

    // Get number of device extensions
    uint32_t deviceExtensionCount;
    REQUIRE(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr) == VK_SUCCESS);

    // Enumerate device extensions
    deviceExtensions.resize(deviceExtensionCount);
    REQUIRE(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, deviceExtensions.data()) == VK_SUCCESS);
}

VkBool32 Device::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
    WARN(pCallbackData->pMessage);
    return VK_FALSE;
}

void Device::CreateDevice() {
    // Get the supported features
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

    // Enable the selected set of features
    VkPhysicalDeviceFeatures enabledFeatures{};

    // Get the number of families
    uint32_t queueFamilyPropertyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

    // Get all families
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

    // Default queue priority
    float queuePriorities = 1.0f;

    // All queue creations
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    // Find optimal queues
    for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
        const VkQueueFamilyProperties &family = queueFamilyProperties[i];

        // Standard graphics & compute queue
        const VkQueueFlags kGraphicsFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;

        // Test candidates
        if (family.queueCount && (family.queueFlags & kGraphicsFlags) == kGraphicsFlags) {
            graphicsQueue.family = i;
        } else if (family.queueCount && (family.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            computeQueue.family = i;
        } else if (family.queueCount && (family.queueFlags & VK_QUEUE_TRANSFER_BIT)) {
            transferQueue.family = i;
        }
    }

    // Queue creation
    if (graphicsQueue.family != ~0u) {
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueCount = 1;
        queueInfo.queueFamilyIndex = graphicsQueue.family;
        queueInfo.pQueuePriorities = &queuePriorities;
        queueCreateInfos.push_back(queueInfo);
    }

    // Queue creation
    if (computeQueue.family != ~0u) {
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueCount = 1;
        queueInfo.queueFamilyIndex = computeQueue.family;
        queueInfo.pQueuePriorities = &queuePriorities;
        queueCreateInfos.push_back(queueInfo);
    }

    // Queue creation
    if (transferQueue.family != ~0u) {
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueCount = 1;
        queueInfo.queueFamilyIndex = transferQueue.family;
        queueInfo.pQueuePriorities = &queuePriorities;
        queueCreateInfos.push_back(queueInfo);
    }

    // Must be assigned
    REQUIRE(graphicsQueue.family != ~0u);

    // Create the device
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.enabledExtensionCount = 0;
    deviceCreateInfo.ppEnabledExtensionNames = nullptr;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
    deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    REQUIRE(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) == VK_SUCCESS);

    // Get the allocated queue
    if (graphicsQueue.family != ~0u) {
        vkGetDeviceQueue(device, graphicsQueue.family, 0, &graphicsQueue.queue);
    }

    // Queue creation
    if (computeQueue.family != ~0u) {
        vkGetDeviceQueue(device, computeQueue.family, 0, &computeQueue.queue);
    }

    // Queue creation
    if (transferQueue.family != ~0u) {
        vkGetDeviceQueue(device, transferQueue.family, 0, &transferQueue.queue);
    }
}

void Device::CreateSharedDescriptorPool() {
    // Standard pool
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 512},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 512},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 512},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 512},
    };

    // Create info
    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptorPoolInfo.pPoolSizes = poolSizes;
    descriptorPoolInfo.poolSizeCount = 4;
    descriptorPoolInfo.maxSets = 512;

    // Attempt to create the pool
    REQUIRE(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &sharedDescriptorPool) == VK_SUCCESS);
}

void Device::CreateSharedQueuePool(QueueInfo& info) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = info.family;

    // Attempt to create the pool
    REQUIRE(vkCreateCommandPool(device, &poolInfo, nullptr, &info.sharedCommandPool) == VK_SUCCESS);
}

void Device::CreateSharedQueuePools() {
    if (graphicsQueue.family != ~0u) {
        CreateSharedQueuePool(graphicsQueue);
    }

    if (computeQueue.family != ~0u) {
        CreateSharedQueuePool(computeQueue);
    }

    if (transferQueue.family != ~0u) {
        CreateSharedQueuePool(transferQueue);
    }
}

Device::~Device() {
    // Release all resources
    ReleaseResources();

    // Release shared pools
    ReleaseShared();

    // Release device
    vkDestroyDevice(device, nullptr);

    // Release validation messenger if needed
    if (debugMessenger) {
        // Get release proc
        auto next_vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        REQUIRE(next_vkDestroyDebugUtilsMessengerEXT);

        // Release it
        next_vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    // Release instance
    vkDestroyInstance(instance, nullptr);
}

void Device::ReleaseResources() {
    // Destroy command buffers
    for (CommandBufferInfo& info : commandBuffers) {
        vkFreeCommandBuffers(device, info.pool, 1, &info.commandBuffer);
    }

    // Destroy pipelines
    for (PipelineInfo& info : pipelines) {
        vkDestroyPipeline(device, info.pipeline, nullptr);
        vkDestroyPipelineLayout(device, info.layout, nullptr);
    }

    // Destroy sets
    for (ResourceSetInfo& info : resourceSets) {
        vkFreeDescriptorSets(device, sharedDescriptorPool, 1, &info.set);
    }

    // Destroy layouts
    for (ResourceLayoutInfo& info : resourceLayouts) {
        vkDestroyDescriptorSetLayout(device, info.layout, nullptr);
    }

    // Destroy resources
    for (ResourceInfo& info : resources) {
        switch (info.type) {
            case ResourceType::TexelBuffer:
            case ResourceType::RWTexelBuffer:
                vkDestroyBufferView(device, info.texelBuffer.view, nullptr);
                vmaDestroyBuffer(allocator, info.texelBuffer.buffer, info.texelBuffer.allocation);
                break;
            case ResourceType::Texture1D:
            case ResourceType::RWTexture1D:
            case ResourceType::Texture2D:
            case ResourceType::RWTexture2D:
            case ResourceType::Texture3D:
            case ResourceType::RWTexture3D:
                vkDestroyImageView(device, info.texture.view, nullptr);
                vmaDestroyImage(allocator, info.texture.image, info.texture.allocation);
                break;
        }
    }

    // Destroy allocator
    vmaDestroyAllocator(allocator);
}

void Device::ReleaseShared() {
    // Descriptors
    vkDestroyDescriptorPool(device, sharedDescriptorPool, nullptr);

    // Graphics
    if (graphicsQueue.family != ~0u) {
        vkDestroyCommandPool(device, graphicsQueue.sharedCommandPool, nullptr);
    }

    // Compute
    if (computeQueue.family != ~0u) {
        vkDestroyCommandPool(device, computeQueue.sharedCommandPool, nullptr);
    }

    // Transfer
    if (transferQueue.family != ~0u) {
        vkDestroyCommandPool(device, transferQueue.sharedCommandPool, nullptr);
    }
}

void Device::CreateAllocator() {
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.instance = instance;
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    REQUIRE(vmaCreateAllocator(&allocatorInfo, &allocator) == VK_SUCCESS);
}

QueueID Device::GetQueue(QueueType type) {
    switch (type) {
        case QueueType::Graphics:
            return QueueID(0);
        case QueueType::Compute:
            return QueueID(1);
        case QueueType::Transfer:
            return QueueID(2);
    }

    return QueueID::Invalid();
}

BufferID Device::CreateTexelBuffer(ResourceType type, Backend::IL::Format format, uint64_t size, void *data) {
    ResourceInfo& resource = resources.emplace_back();
    resource.type = type;

    // Buffer info
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;

    switch (type) {
        default:
            ASSERT(false, "Invalid type");
            break;
        case ResourceType::TexelBuffer:
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
            break;
        case ResourceType::RWTexelBuffer:
            bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
            break;
    }

    // Allocation info
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    // Attempt to allocate and create buffer
    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &resource.texelBuffer.buffer, &resource.texelBuffer.allocation, nullptr);

    // View info
    VkBufferViewCreateInfo bufferViewInfo{};
    bufferViewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    bufferViewInfo.buffer = resource.texelBuffer.buffer;
    bufferViewInfo.format = Translate(format);
    bufferViewInfo.range = size;

    // Attempt to create buffer view
    REQUIRE(vkCreateBufferView(device, &bufferViewInfo, nullptr, &resource.texelBuffer.view) == VK_SUCCESS);

    return BufferID(ResourceID(resources.size() - 1));
}

TextureID Device::CreateTexture(ResourceType type, Backend::IL::Format format, uint32_t width, uint32_t height, uint32_t depth, void *data) {
    ResourceInfo& resource = resources.emplace_back();
    resource.type = type;

    // Image creation info
    VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.extent = VkExtent3D{width, height, depth};
    imageInfo.arrayLayers = 1;
    imageInfo.format = Translate(format);
    imageInfo.mipLevels = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    switch (type) {
        default:
            ASSERT(false, "Invalid type");
            break;
        case ResourceType::Texture1D:
            imageInfo.imageType = VK_IMAGE_TYPE_1D;
            imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
            break;
        case ResourceType::Texture2D:
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
            break;
        case ResourceType::Texture3D:
            imageInfo.imageType = VK_IMAGE_TYPE_3D;
            imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
            break;
        case ResourceType::RWTexture1D:
            imageInfo.imageType = VK_IMAGE_TYPE_1D;
            imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
            break;
        case ResourceType::RWTexture2D:
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
            break;
        case ResourceType::RWTexture3D:
            imageInfo.imageType = VK_IMAGE_TYPE_3D;
            imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
            break;
    }

    // Resides on GPU
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    // Attempt to allocate and create image
    vmaCreateImage(allocator, &imageInfo, &allocInfo, &resource.texture.image, &resource.texture.allocation, nullptr);

    // View creation info
    VkImageViewCreateInfo imageViewInfo{};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.image = resource.texture.image;
    imageViewInfo.format = imageInfo.format;
    imageViewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.layerCount = 1;
    imageViewInfo.subresourceRange.levelCount = 1;

    switch (type) {
        default:
        ASSERT(false, "Invalid type");
            break;
        case ResourceType::Texture1D:
        case ResourceType::RWTexture1D:
            imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            break;
        case ResourceType::Texture2D:
        case ResourceType::RWTexture2D:
            imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            break;
        case ResourceType::Texture3D:
        case ResourceType::RWTexture3D:
            imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
    }

    // Attempt to create view
    REQUIRE(vkCreateImageView(device, &imageViewInfo, nullptr, &resource.texture.view) == VK_SUCCESS);

    // Texture id
    TextureID id(ResourceID(resources.size() - 1));

    // Enqueue command
    UpdateCommand command;
    command.texture.type = UpdateCommandType::TransitionTexture;
    command.texture.id = id;
    updateCommands.push_back(command);

    return id;
}

ResourceLayoutID Device::CreateResourceLayout(const ResourceType *types, uint32_t count) {
    ResourceLayoutInfo& info = resourceLayouts.emplace_back();
    info.resources.insert(info.resources.end(), types, types + count);

    std::vector<VkDescriptorSetLayoutBinding> bindings;

    // Translate bindings
    for (uint32_t i = 0; i < count; i++) {
        VkDescriptorSetLayoutBinding& binding = bindings.emplace_back() = {};
        binding.binding = i;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        switch (types[i]) {
            case ResourceType::TexelBuffer:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                break;
            case ResourceType::RWTexelBuffer:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                break;
            case ResourceType::Texture1D:
            case ResourceType::Texture2D:
            case ResourceType::Texture3D:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                break;
            case ResourceType::RWTexture1D:
            case ResourceType::RWTexture2D:
            case ResourceType::RWTexture3D:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                break;
        }
    }

    // Set layout create info
    VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{};
    descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayoutInfo.bindingCount = bindings.size();
    descriptorLayoutInfo.pBindings = bindings.data();

    // Attempt to create
    REQUIRE(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &info.layout) == VK_SUCCESS);

    return Test::ResourceLayoutID(resourceLayouts.size() - 1);
}

ResourceSetID Device::CreateResourceSet(ResourceLayoutID layout, const ResourceID *setResources, uint32_t count) {
    ResourceSetInfo& info = resourceSets.emplace_back();

    // Set allocation info
    VkDescriptorSetAllocateInfo setInfo{};
    setInfo.sType  = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setInfo.pSetLayouts = &resourceLayouts.at(layout).layout;
    setInfo.descriptorPool = sharedDescriptorPool;
    setInfo.descriptorSetCount = 1;

    // Attempt to allocate
    REQUIRE(vkAllocateDescriptorSets(device, &setInfo, &info.set) == VK_SUCCESS);

    // Temp
    std::vector<VkDescriptorImageInfo> imageInfos;
    std::vector<VkWriteDescriptorSet> writes;

    // Translate additional descriptor infos
    for (uint32_t i = 0; i < count; i++) {
        ResourceType type = resourceLayouts.at(layout).resources[i];

        // Skip
        switch (type) {
            default:
                continue;
            case ResourceType::Texture1D:
            case ResourceType::RWTexture1D:
            case ResourceType::Texture2D:
            case ResourceType::RWTexture2D:
            case ResourceType::Texture3D:
            case ResourceType::RWTexture3D:
                break;
        }

        VkDescriptorImageInfo& imageInfo = imageInfos.emplace_back();
        imageInfo.imageView = resources.at(setResources[i]).texture.view;
        imageInfo.sampler = nullptr;

        switch (type) {
            default:
                ASSERT(false, "Invalid type");
                break;
            case ResourceType::Texture1D:
            case ResourceType::Texture2D:
            case ResourceType::Texture3D:
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                break;
            case ResourceType::RWTexture1D:
            case ResourceType::RWTexture2D:
            case ResourceType::RWTexture3D:
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                break;
        }
    }

    // Current offsets
    uint32_t imageOffset{0};

    // Translate writes
    for (uint32_t i = 0; i < count; i++) {
        VkWriteDescriptorSet& write = writes.emplace_back() = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.descriptorCount = 1;
        write.dstBinding = i;
        write.dstSet = info.set;

        switch (resourceLayouts.at(layout).resources[i]) {
            case ResourceType::TexelBuffer:
                write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                write.pTexelBufferView = &resources.at(setResources[i]).texelBuffer.view;
                break;
            case ResourceType::RWTexelBuffer:
                write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                write.pTexelBufferView = &resources.at(setResources[i]).texelBuffer.view;
                break;
            case ResourceType::Texture1D:
            case ResourceType::Texture2D:
            case ResourceType::Texture3D:
                write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                write.pImageInfo = &imageInfos[imageOffset++];
                break;
            case ResourceType::RWTexture1D:
            case ResourceType::RWTexture2D:
            case ResourceType::RWTexture3D:
                write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                write.pImageInfo = &imageInfos[imageOffset++];
                break;
        }
    }

    // Submit writes
    vkUpdateDescriptorSets(device, count, writes.data(), 0, nullptr);

    return Test::ResourceSetID(resourceSets.size() - 1);
}

PipelineID Device::CreateComputePipeline(const ResourceLayoutID* layouts, uint32_t layoutCount, const void *shaderCode, uint32_t shaderSize) {
    PipelineInfo& info = pipelines.emplace_back();

    // Shader info
    VkShaderModuleCreateInfo moduleCreateInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    moduleCreateInfo.codeSize = shaderSize;
    moduleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode);

    // Attempt to create a shader
    VkShaderModule module;
    REQUIRE(vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &module) == VK_SUCCESS);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.pName = "main";
    stageInfo.module = module;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;

    // Unwrap layouts
    std::vector<VkDescriptorSetLayout> vkLayouts;
    for (uint32_t i = 0; i < layoutCount; i++) {
        vkLayouts.push_back(resourceLayouts.at(layouts[i]).layout);
    }

    // Layout create info
    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.setLayoutCount = vkLayouts.size();
    layoutInfo.pSetLayouts = vkLayouts.data();

    // Attempt to create layout
    REQUIRE(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &info.layout) == VK_SUCCESS);

    // Pipeline info
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = info.layout;
    pipelineInfo.stage  = stageInfo;

    // Attempt to create pipeline
    REQUIRE(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &info.pipeline) == VK_SUCCESS);

    // Free module
    vkDestroyShaderModule(device, module, nullptr);

    return Test::PipelineID(pipelines.size() - 1);
}

CommandBufferID Device::CreateCommandBuffer(QueueType type) {
    CommandBufferInfo& info = commandBuffers.emplace_back();

    // Set pool
    switch (type) {
        case QueueType::Graphics:
            info.pool = graphicsQueue.sharedCommandPool;
            break;
        case QueueType::Compute:
            info.pool = computeQueue.sharedCommandPool;
            break;
        case QueueType::Transfer:
            info.pool = transferQueue.sharedCommandPool;
            break;
    }

    // Allocation info
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;
    allocateInfo.commandPool        = info.pool;

    // Attempt to allocate the command buffer
    REQUIRE(vkAllocateCommandBuffers(device, &allocateInfo, &info.commandBuffer) == VK_SUCCESS);

    return Test::CommandBufferID(commandBuffers.size() - 1);
}

void Device::BeginCommandBuffer(CommandBufferID commandBuffer) {
    CommandBufferInfo& info = commandBuffers.at(commandBuffer);
    info.context = {};

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    REQUIRE(vkBeginCommandBuffer(info.commandBuffer, &beginInfo) == VK_SUCCESS);
}

void Device::EndCommandBuffer(CommandBufferID commandBuffer) {
    vkEndCommandBuffer(commandBuffers.at(commandBuffer).commandBuffer);
}

void Device::BindPipeline(CommandBufferID commandBuffer, PipelineID pipeline) {
    CommandBufferInfo& info = commandBuffers.at(commandBuffer);
    info.context.pipeline = pipeline;

    vkCmdBindPipeline(info.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.at(pipeline).pipeline);
}

void Device::BindResourceSet(CommandBufferID commandBuffer, ResourceSetID resourceSet) {
    CommandBufferInfo& info = commandBuffers.at(commandBuffer);

    vkCmdBindDescriptorSets(
        info.commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        pipelines.at(info.context.pipeline).layout,
        0, 1, &resourceSets.at(resourceSet).set,
        0, nullptr
    );
}

void Device::Dispatch(CommandBufferID commandBuffer, uint32_t x, uint32_t y, uint32_t z) {
    vkCmdDispatch(commandBuffers.at(commandBuffer).commandBuffer, x, y, z);
}

void Device::Submit(QueueID queueID, CommandBufferID commandBuffer) {
    // Determine the queue
    VkQueue queue{};
    switch (queueID) {
        case 0:
            queue = graphicsQueue.queue;
            break;
        case 1:
            queue = computeQueue.queue;
            break;
        case 2:
            queue = transferQueue.queue;
            break;
    }

    // Submit the command buffer
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pCommandBuffers = &commandBuffers.at(commandBuffer).commandBuffer;
    submit.commandBufferCount = 1;
    REQUIRE(vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE) == VK_SUCCESS);
}

void Device::Flush() {
    vkDeviceWaitIdle(device);
}

void Device::InitializeResources(CommandBufferID commandBuffer) {
    CommandBufferInfo& info = commandBuffers.at(commandBuffer);

    for (const UpdateCommand& cmd : updateCommands) {
        switch (cmd.type) {
            case UpdateCommandType::TransitionTexture: {
                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.layerCount = 1;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.srcAccessMask = VK_ACCESS_NONE_KHR;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.image = resources.at(cmd.texture.id.value).texture.image;

                vkCmdPipelineBarrier(
                    info.commandBuffer,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    0x0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );
                break;
            }
        }
    }
}
