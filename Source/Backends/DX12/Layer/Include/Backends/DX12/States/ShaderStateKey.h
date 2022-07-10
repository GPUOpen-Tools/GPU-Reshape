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
        if (hash != rhs.hash || byteCode.BytecodeLength != rhs.byteCode.BytecodeLength) {
            return false;
        }

        return !std::memcmp(byteCode.pShaderBytecode, rhs.byteCode.pShaderBytecode, byteCode.BytecodeLength);
    }

    /// User byteCode
    D3D12_SHADER_BYTECODE byteCode;

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