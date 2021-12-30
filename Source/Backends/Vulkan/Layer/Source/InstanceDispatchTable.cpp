#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>
#include <Backends/Vulkan/Instance.h>

// Std
#include <cstring>

/// Global instantiation
std::mutex                                InstanceDispatchTable::Mutex;
std::map<void *, InstanceDispatchTable *> InstanceDispatchTable::Table;

void InstanceDispatchTable::Populate(VkInstance instance, PFN_vkGetInstanceProcAddr getProcAddr) {
    object                     = instance;
    next_vkGetInstanceProcAddr = getProcAddr;

    // Populate instance
    next_vkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(getProcAddr(instance, "vkDestroyInstance"));
    next_vkGetPhysicalDeviceMemoryProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(getProcAddr(object, "vkGetPhysicalDeviceMemoryProperties"));
    next_vkGetPhysicalDeviceMemoryProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties2KHR>(getProcAddr(object, "vkGetPhysicalDeviceMemoryProperties2KHR"));
    next_vkGetPhysicalDeviceProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(getProcAddr(object, "vkGetPhysicalDeviceProperties"));
    next_vkGetPhysicalDeviceFeatures2 = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(getProcAddr(object, "vkGetPhysicalDeviceFeatures2"));
}

PFN_vkVoidFunction InstanceDispatchTable::GetHookAddress(const char *name) {
    if (!std::strcmp(name, "vkCreateInstance"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateInstance);

    if (!std::strcmp(name, "vkDestroyInstance"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyInstance);

    if (!std::strcmp(name, "vkEnumerateInstanceLayerProperties"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkEnumerateInstanceLayerProperties);

    if (!std::strcmp(name, "vkEnumerateInstanceExtensionProperties"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkEnumerateInstanceExtensionProperties);

    // No hook
    return nullptr;
}
