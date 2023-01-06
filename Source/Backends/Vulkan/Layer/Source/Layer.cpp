#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>

// Vulkan
#include <vulkan/vk_layer.h>

// Std
#include <cstring>
#include <mutex>
#include <map>
#include <vector>
#include <intrin.h>

/// Linkage information
#if defined(_MSC_VER)
#	define EXPORT_C __declspec(dllexport)
#else
#	define EXPORT_C __attribute__((visibility("default")))
#endif

// All exports
extern "C" {
    [[maybe_unused]]
    EXPORT_C VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL Hook_vkGetDeviceProcAddr(VkDevice device, const char *pName) {
        if (!std::strcmp(pName, "vkGetDeviceProcAddr")) {
            return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkGetDeviceProcAddr);
        }
        
        // Device table
        if (PFN_vkVoidFunction hook = DeviceDispatchTable::GetHookAddress(pName)) {
            return hook;
        }

        // Instance table
        if (PFN_vkVoidFunction hook = InstanceDispatchTable::GetHookAddress(pName)) {
            return hook;
        }

        // Attempt to get table
        auto table = DeviceDispatchTable::Get(GetInternalTable(device));
        if (!table) {
            return nullptr;
        }

        // Pass down callchain
        return table->next_vkGetDeviceProcAddr(device, pName);
    }

    [[maybe_unused]]
    EXPORT_C VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL Hook_vkGetInstanceProcAddr(VkInstance instance, const char *pName) {
        if (!std::strcmp(pName, "vkGetInstanceProcAddr")) {
            return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkGetInstanceProcAddr);
        }

        // Instance table
        if (PFN_vkVoidFunction hook = InstanceDispatchTable::GetHookAddress(pName)) {
            return hook;
        }

        // Device table
        if (PFN_vkVoidFunction hook = DeviceDispatchTable::GetHookAddress(pName)) {
            return hook;
        }

        // Attempt to get table
        auto table = InstanceDispatchTable::Get(GetInternalTable(instance));
        if (!table) {
            return nullptr;
        }

        // Pass down callchain
        return table->next_vkGetInstanceProcAddr(instance, pName);
    }

    [[maybe_unused]]
    EXPORT_C VKAPI_ATTR VkResult VKAPI_CALL Hook_vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface *pVersionStruct) {
        if (pVersionStruct->loaderLayerInterfaceVersion >= 2) {
            pVersionStruct->pfnGetInstanceProcAddr       = Hook_vkGetInstanceProcAddr;
            pVersionStruct->pfnGetDeviceProcAddr         = Hook_vkGetDeviceProcAddr;
            pVersionStruct->pfnGetPhysicalDeviceProcAddr = nullptr;
        }

        if (pVersionStruct->loaderLayerInterfaceVersion > CURRENT_LOADER_LAYER_INTERFACE_VERSION) {
            pVersionStruct->loaderLayerInterfaceVersion = CURRENT_LOADER_LAYER_INTERFACE_VERSION;
        }

        return VK_SUCCESS;
    }
}