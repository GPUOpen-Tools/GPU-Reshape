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

// Layer
#include <Backends/DX12/States/ShaderStateKey.h>

// Common
#include <Common/Hash.h>

// Std
#include <map>

// Forward declarations
struct ShaderState;

class ShaderSet {
public:
    /// Get a shader state
    /// \param key shader key
    /// \return nullptr if not found
    ShaderState* Get(const ShaderStateKey& key) {
        auto it = states.find(key);
        if (it == states.end()) {
            return nullptr;
        }

        return it->second;
    }

    /// Add a new shader state
    /// \param key shader key
    /// \param state shader state
    void Add(const ShaderStateKey& key, ShaderState* state) {
        ASSERT(!states.contains(key), "Duplicate key");
        states[key] = state;
    }

    /// Remove a shader state
    /// \param key shader key
    void Remove(const ShaderStateKey& key) {
        ASSERT(states.contains(key), "Missing key");
        states.erase(key);
    }

private:
    /// All mapped states
    std::map<ShaderStateKey, ShaderState *> states;
};
