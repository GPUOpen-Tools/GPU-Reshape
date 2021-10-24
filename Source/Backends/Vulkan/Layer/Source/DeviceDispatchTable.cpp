#include <Backends/Vulkan/DeviceDispatchTable.h>

void DeviceDispatchTable::Populate(VkDevice device, PFN_vkGetDeviceProcAddr getProcAddr) {
    // Populate all generated commands
    commandBufferDispatchTable.Populate(device, getProcAddr);
}

PFN_vkVoidFunction DeviceDispatchTable::GetHookAddress(const char *name) {
    if (PFN_vkVoidFunction hook = CommandBufferDispatchTable::GetHookAddress(name)) {
        return hook;
    }

    // No hook
    return nullptr;
}
