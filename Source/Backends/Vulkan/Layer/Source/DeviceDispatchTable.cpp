#include <Backends/Vulkan/DeviceDispatchTable.h>
#include <Backends/Vulkan/Device.h>

/// Global instantiation
std::mutex                              DeviceDispatchTable::Mutex;
std::map<void *, DeviceDispatchTable *> DeviceDispatchTable::Table;

void DeviceDispatchTable::Populate(VkDevice device, PFN_vkGetInstanceProcAddr getInstanceProcAddr, PFN_vkGetDeviceProcAddr getDeviceProcAddr) {
    object                     = device;
    next_vkGetInstanceProcAddr = getInstanceProcAddr;
    next_vkGetDeviceProcAddr   = getDeviceProcAddr;

    // Populate device
    next_vkDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(getDeviceProcAddr(device, "vkDestroyDevice"));

    // Populate all generated commands
    commandBufferDispatchTable.Populate(device, getDeviceProcAddr);
}

PFN_vkVoidFunction DeviceDispatchTable::GetHookAddress(const char *name) {
    if (!std::strcmp(name, "vkCreateDevice"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateDevice);

    if (!std::strcmp(name, "vkDestroyDevice"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyDevice);

    if (!std::strcmp(name, "vkEnumerateDeviceLayerProperties"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkEnumerateDeviceLayerProperties);

    if (!std::strcmp(name, "vkEnumerateDeviceExtensionProperties"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkEnumerateDeviceExtensionProperties);

    // Check command hooks
    if (PFN_vkVoidFunction hook = CommandBufferDispatchTable::GetHookAddress(name)) {
        return hook;
    }

    // No hook
    return nullptr;
}
