#pragma once

// Layer
#include <Backends/DX12/DX12.h>

// Forward declarations
struct DeviceState;
struct CommandBuffer;
struct CommandListState;
struct ShaderExportStreamState;

/// Commit a set of commands
/// \param device parent device
/// \param commandList command list to be committed on
/// \param buffer user command buffer to commit from
/// \param streamState current streaming state
void CommitCommands(DeviceState* device, ID3D12GraphicsCommandList* commandList, const CommandBuffer& buffer, ShaderExportStreamState* streamState);

/// Commit all pending commands
/// \param commandList command list to be executed on
void CommitCommands(CommandListState* state);
