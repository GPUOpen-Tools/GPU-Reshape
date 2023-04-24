#pragma once

// Test
#include "ShaderBlob.h"

// Common
#include <Common/Assert.h>

// Std
#include <unordered_map>
#include <string>

class ShaderType {
public:
    /// Get a shader
    /// \param name device name
    /// \return shader blob
    const ShaderBlob& Get(const std::string& name) {
        ASSERT(blobs.contains(name), "Shader device not found");
        return blobs.at(name);
    }

    /// Register a shader
    /// \param device device name
    /// \param blob shader blob
    void Register(const std::string& device, const ShaderBlob& blob) {
        if (auto it = blobs.find(device); it != blobs.end()) {
            ASSERT(
                it->second.length == blob.length && !std::memcmp(blob.code, it->second.code, blob.length),
               "Duplicate blob registered with differing data"
           );
        }

        // Valid, add
        blobs[device] = blob;
    }

private:
    /// All blobs
    std::unordered_map<std::string, ShaderBlob> blobs;
};
