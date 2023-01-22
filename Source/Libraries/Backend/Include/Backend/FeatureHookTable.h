#pragma once

// Common
#include <Common/Delegate.h>

// Std
#include <cstdint>

// Forward declarations
class CommandContext;
struct BufferDescriptor;

// Hook types
namespace Hooks {
    /// Invocations
    using DrawInstanced = Delegate<void(CommandContext* context, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)>;
    using DrawIndexedInstanced = Delegate<void(CommandContext* context, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)>;
    using Dispatch = Delegate<void(CommandContext* context, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)>;

    /// Resource
    using CopyBuffer = Delegate<void(CommandContext* context, const BufferDescriptor& source, const BufferDescriptor& dest, uint64_t byteSize)>;
}

/// Contains the required hooks for a given feature
class FeatureHookTable {
public:
    Hooks::DrawInstanced drawInstanced;
    Hooks::DrawIndexedInstanced drawIndexedInstanced;
    Hooks::Dispatch dispatch;
    Hooks::CopyBuffer copyBuffer;
};
