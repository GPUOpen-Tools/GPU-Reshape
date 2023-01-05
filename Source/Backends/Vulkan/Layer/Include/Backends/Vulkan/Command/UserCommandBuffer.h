#pragma once

// Forward declarations
struct CommandBufferObject;

/// Commit all pending commands
/// \param commandBuffer buffer to be executed on
void CommitCommands(CommandBufferObject* commandBuffer);
