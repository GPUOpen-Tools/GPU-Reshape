#pragma once

// Common
#include <Common/Enum.h>

enum class PluginResolveFlag {
    None = 0,

    /// Continue plugin installation on failure
    ContinueOnFailure = BIT(1),
};

/// Declare set
BIT_SET(PluginResolveFlag);
