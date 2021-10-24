#pragma once

#include <vulkan/vulkan_core.h>

struct CommandBufferDispatchTable;

struct CommandBufferObject
{
    VkCommandBuffer             Object;
    CommandBufferDispatchTable* Table;
};
