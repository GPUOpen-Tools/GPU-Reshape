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
    CBuffer
};

struct ResourceInitialization {
    /// Resource sizes (x, [y, [z]])
    std::vector<int64_t> sizes;
};

struct Resource {
    /// Type of this resource
    ResourceType type{ResourceType::None};

    /// View and data format
    std::string_view format;

    /// Initialization info
    ResourceInitialization initialization;
};
