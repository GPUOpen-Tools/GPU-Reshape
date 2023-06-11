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
