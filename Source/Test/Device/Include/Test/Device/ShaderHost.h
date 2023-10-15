// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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
