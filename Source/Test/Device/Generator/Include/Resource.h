#pragma once

#include <string_view>
#include <vector>

enum class ResourceType {
    None,
    Buffer,
    RWBuffer,
    Texture1D,
    RWTexture1D,
    Texture2D,
    RWTexture2D,
    Texture3D,
    RWTexture3D,
    SamplerState,
    StaticSamplerState,
    CBuffer
};

struct ResourceInitialization {
    /// Resource sizes (x, [y, [z]])
    std::vector<int64_t> sizes;

    /// Resource data (a, b, c, d...)
    std::vector<int64_t> data;
};

struct Resource {
    /// Type of this resource
    ResourceType type{ResourceType::None};

    /// View and data format
    std::string_view format;

    /// Optional array size (0 indicates no array)
    uint32_t arraySize{0};

    /// Initialization info
    ResourceInitialization initialization;
};
