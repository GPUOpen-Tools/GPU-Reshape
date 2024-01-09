// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
