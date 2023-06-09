#pragma once

// Layer
#include <Backends/DX12/DX12.h>

// Std
#include <memory>

struct ShaderStateKey {
    /// Comparator
    /// \param rhs
    /// \return
    bool operator==(const ShaderStateKey& rhs) const {
        return hash == rhs.hash;
    }

    /// Bytecode hash
    uint64_t hash{};
};

namespace std {
    template<>
    struct hash<ShaderStateKey> {
        std::size_t operator()(const ShaderStateKey &key) const {
            return key.hash;
        }
    };

    template<>
    struct less<ShaderStateKey> {
        bool operator()(const ShaderStateKey &a, const ShaderStateKey &b) const {
            return a.hash < b.hash;
        }
    };
}