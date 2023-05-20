#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>

// Forward declarations
struct DeviceDispatchTable;
struct CommandBuffer;
struct CommandBufferObject;
struct ShaderExportStreamState;

/// Commit all pending commands
/// \param device parent device
/// \param commandBuffer command buffer to be filled
/// \param buffer user commands to fill from
/// \param streamState streaming state
void CommitCommands(DeviceDispatchTable* device, VkCommandBuffer commandBuffer, const CommandBuffer& buffer, ShaderExportStreamState* streamState);

/// Commit all pending commands
/// \param commandBuffer buffer to be executed on
void CommitCommands(CommandBufferObject* commandBuffer);
