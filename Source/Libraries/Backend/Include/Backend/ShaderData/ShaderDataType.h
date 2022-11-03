#pragma once

#include <Common/Enum.h>

enum class ShaderDataType {
    None,
    Buffer = BIT(0),
    Texture = BIT(1),
    Event = BIT(2),

    /// Descriptor occupying mask
    DescriptorMask = Buffer | Texture,

    /// All data
    All = Buffer | Texture | Event
};

BIT_SET(ShaderDataType);
