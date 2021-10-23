#include "Loader.h"
#include <cstdlib>
#include <filesystem>

Loader::Loader() {
    // Redirect layer path
    std::filesystem::path cwd = std::filesystem::current_path();
   _putenv_s("VK_LAYER_PATH", (cwd.string()).c_str());

    // Get number of instance layers
    uint32_t instancelayerCount;
    REQUIRE(vkEnumerateInstanceLayerProperties(&instancelayerCount, nullptr) == VK_SUCCESS);

    // Enumerate instance layers
    instanceLayers.resize(instancelayerCount);
    REQUIRE(vkEnumerateInstanceLayerProperties(&instancelayerCount, instanceLayers.data()) == VK_SUCCESS);

    // Get number of instance extensions
    uint32_t instanceExtensionCount;
    REQUIRE(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr) == VK_SUCCESS);

    // Enumerate instance extensions
    instanceExtensions.resize(instanceExtensionCount);
    REQUIRE(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensions.data()) == VK_SUCCESS);
}

bool Loader::SupportsInstanceLayer(const char *name) const {
    return std::any_of(instanceLayers.begin(), instanceLayers.end(), [&](const VkLayerProperties& properties)
    {
        return std::strcmp(properties.layerName, name) == 0;
    });
}

bool Loader::SupportsInstanceExtension(const char *name) const {
    return std::any_of(instanceExtensions.begin(), instanceExtensions.end(), [&](const VkExtensionProperties& properties)
    {
        return std::strcmp(properties.extensionName, name) == 0;
    });
}

bool Loader::SupportsDeviceExtension(const char *name) const {
    return std::any_of(deviceExtensions.begin(), deviceExtensions.end(), [&](const VkExtensionProperties& properties)
    {
        return std::strcmp(properties.extensionName, name) == 0;
    });
}

bool Loader::AddInstanceLayer(const char *name) {
    if (!SupportsInstanceLayer(name))
        return false;

    enabledInstanceLayers.push_back(name);
    return true;
}

bool Loader::AddInstanceExtension(const char *name) {
    if (!SupportsInstanceExtension(name))
        return false;

    enabledInstanceExtensions.push_back(name);
    return true;
}

bool Loader::AddDeviceExtension(const char *name) {
    if (!SupportsDeviceExtension(name))
        return false;

    enabledDeviceExtensions.push_back(name);
    return true;
}

void Loader::CreateInstance() {
    // General app info
    VkApplicationInfo applicationInfo{};
    applicationInfo.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.apiVersion       = VK_API_VERSION_1_2;
    applicationInfo.pApplicationName = "GPUOpen GBV";
    applicationInfo.pEngineName      = "GPUOpen GBV";

    // Instance info
    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo        = &applicationInfo;
    instanceCreateInfo.enabledExtensionCount   = enabledInstanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = enabledInstanceExtensions.data();
    instanceCreateInfo.enabledLayerCount       = enabledInstanceLayers.size();
    instanceCreateInfo.ppEnabledLayerNames     = enabledInstanceLayers.data();
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

    // Get number of device extensions
    uint32_t deviceExtensionCount;
    REQUIRE(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr) == VK_SUCCESS);

    // Enumerate device extensions
    deviceExtensions.resize(deviceExtensionCount);
    REQUIRE(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, deviceExtensions.data()) == VK_SUCCESS);
}

void Loader::CreateDevice() {
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

    // Queue creation
    VkDeviceQueueCreateInfo primaryQueueInfo{};
    primaryQueueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    primaryQueueInfo.queueCount       = 1;
    primaryQueueInfo.queueFamilyIndex = 0xFFFFFFFF;
    primaryQueueInfo.pQueuePriorities = &queuePriorities;

    // Find optimal queue
    for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
        const VkQueueFamilyProperties& family  = queueFamilyProperties[i];

        // Valid candidate for Graphics & Compute?
        if (family.queueCount && (family.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)))
        {
            primaryQueueInfo.queueFamilyIndex = i;
        }
    }

    // Must be assigned
    REQUIRE(primaryQueueInfo.queueFamilyIndex != 0xFFFFFFFF);
    queueFamilyIndex = primaryQueueInfo.queueFamilyIndex;

    // Create the device
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.enabledExtensionCount   = enabledDeviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = enabledDeviceExtensions.data();
    deviceCreateInfo.queueCreateInfoCount    = 1;
    deviceCreateInfo.pEnabledFeatures        = &enabledFeatures;
    deviceCreateInfo.queueCreateInfoCount    = 1;
    deviceCreateInfo.pQueueCreateInfos       = &primaryQueueInfo;
    REQUIRE(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) == VK_SUCCESS);
}

