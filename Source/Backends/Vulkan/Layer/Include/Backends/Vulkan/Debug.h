#pragma once

#include <Backends/Vulkan/Vulkan.h>

/// Hooks
VkResult Hook_vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo);
