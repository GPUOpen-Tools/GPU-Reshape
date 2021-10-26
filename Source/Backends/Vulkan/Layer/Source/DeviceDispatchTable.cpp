#include <Backends/Vulkan/DeviceDispatchTable.h>
#include <Backends/Vulkan/Device.h>
#include <Backends/Vulkan/CommandBuffer.h>

// Std
#include <cstring>

/// Global instantiation
std::mutex                              DeviceDispatchTable::Mutex;
std::map<void *, DeviceDispatchTable *> DeviceDispatchTable::Table;

void DeviceDispatchTable::Populate(VkDevice device, PFN_vkGetInstanceProcAddr getInstanceProcAddr, PFN_vkGetDeviceProcAddr getDeviceProcAddr) {
    object                     = device;
    next_vkGetInstanceProcAddr = getInstanceProcAddr;
    next_vkGetDeviceProcAddr   = getDeviceProcAddr;

    // Populate device
    next_vkDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(getDeviceProcAddr(device, "vkDestroyDevice"));

    // Populate command buffer
    next_vkCreateCommandPool = reinterpret_cast<PFN_vkCreateCommandPool>(getDeviceProcAddr(device, "vkCreateCommandPool"));
    next_vkAllocateCommandBuffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(getDeviceProcAddr(device, "vkAllocateCommandBuffers"));
    next_vkBeginCommandBuffer = reinterpret_cast<PFN_vkBeginCommandBuffer>(getDeviceProcAddr(device, "vkBeginCommandBuffer"));
    next_vkEndCommandBuffer = reinterpret_cast<PFN_vkEndCommandBuffer>(getDeviceProcAddr(device, "vkEndCommandBuffer"));
    next_vkFreeCommandBuffers = reinterpret_cast<PFN_vkFreeCommandBuffers>(getDeviceProcAddr(device, "vkFreeCommandBuffers"));
    next_vkDestroyCommandPool = reinterpret_cast<PFN_vkDestroyCommandPool>(getDeviceProcAddr(device, "vkDestroyCommandPool"));

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

    if (!std::strcmp(name, "vkCreateCommandPool"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateCommandPool);

    if (!std::strcmp(name, "Hook_vkAllocateCommandBuffers"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkAllocateCommandBuffers);

    if (!std::strcmp(name, "vkBeginCommandBuffer"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkBeginCommandBuffer);

    if (!std::strcmp(name, "vkEndCommandBuffer"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkEndCommandBuffer);

    if (!std::strcmp(name, "vkFreeCommandBuffers"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkFreeCommandBuffers);

    if (!std::strcmp(name, "vkDestroyCommandPool"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyCommandPool);

    // Check command hooks
    if (PFN_vkVoidFunction hook = CommandBufferDispatchTable::GetHookAddress(name)) {
        return hook;
    }

    // No hook
    return nullptr;
}
