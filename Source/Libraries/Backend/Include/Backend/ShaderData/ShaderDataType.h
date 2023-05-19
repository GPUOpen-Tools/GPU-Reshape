#pragma once

#include <Common/Enum.h>

enum class ShaderDataType {
    None,
    Buffer = BIT(0),
    Texture = BIT(1),
    Event = BIT(2),
    Descriptor = BIT(3),

    /// Descriptor occupying mask
    DescriptorMask = Buffer | Texture,

    /// All data
    All = Buffer | Texture | Event | Descriptor
};

BIT_SET(ShaderDataType);
