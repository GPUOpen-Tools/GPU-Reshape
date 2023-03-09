#pragma once

// Backend
#include "ResourceToken.h"

// Forward declarations
struct TextureDescriptor;
struct BufferDescriptor;

struct ResourceInfo {
    /// Create a texture info
    /// \param token given token
    /// \param texture given texture
    /// \return info
    static ResourceInfo Texture(const ResourceToken& token, const TextureDescriptor* texture) {
        return ResourceInfo {
            .token = token,
            .textureDescriptor = texture
        };
    }

    /// Create a buffer info
    /// \param token given token
    /// \param buffer given buffer
    /// \return info
    static ResourceInfo Buffer(const ResourceToken& token, const BufferDescriptor* buffer) {
        return ResourceInfo {
            .token = token,
            .bufferDescriptor = buffer
        };
    }

    /// PRMT token
    ResourceToken token;

    /// Descriptor data
    union {
        const TextureDescriptor* textureDescriptor;
        const BufferDescriptor* bufferDescriptor;
    };
};
