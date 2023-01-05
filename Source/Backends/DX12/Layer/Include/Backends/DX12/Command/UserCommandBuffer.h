#pragma once

// Forward declarations
struct CommandListState;

/// Commit all pending commands
/// \param commandList command list to be executed on
void CommitCommands(CommandListState* commandList);
