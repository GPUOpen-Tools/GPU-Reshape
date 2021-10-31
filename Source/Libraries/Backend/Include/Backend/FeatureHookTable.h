#pragma once

// Common
#include <Common/Delegate.h>

// Hook types
namespace Hooks {
    using DrawIndexed = Delegate<void(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)>;
}

/// Contains the required hooks for a given feature
class FeatureHookTable {
public:
    Hooks::DrawIndexed drawIndexed;
};
