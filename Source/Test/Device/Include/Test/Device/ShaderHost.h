#pragma once

// Test
#include "ShaderType.h"

// Std
#include <unordered_map>
#include <string>

class ShaderHost {
public:
    /// Get a shader
    /// \param name given shader name
    /// \param device device name to fetch for
    /// \return shader blob
    static ShaderBlob Get(const std::string& name, const std::string& device) {
        ShaderHost& self = Get();

        ASSERT(self.shaders.count(name), "Shader type not found");
        return self.shaders.at(name).Get(device);
    }

    /// Register a shader
    /// \param name given shader name
    /// \param device device name to register for
    /// \param blob blob to register
    static void Register(const std::string& name, const std::string& device, const ShaderBlob& blob) {
        ShaderHost& self = Get();
        self.shaders[name].Register(device, blob);
    }

private:
    /// All shader types
    std::unordered_map<std::string, ShaderType> shaders;

    /// Singleton getter
    static ShaderHost& Get();
};
