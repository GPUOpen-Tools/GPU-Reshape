#pragma once

// Backend
#include <Backend/ShaderExport.h>

// Std
#include <cstdint>

struct ShaderSourceMapping {
    using TSortKey = uint64_t;

    /// Get the inline (shader) sort key
    /// \return the sort key
    TSortKey GetInlineSortKey() const {
        TSortKey key{0};
        key |= fileUID;
        key |= static_cast<TSortKey>(line) << 16ull;
        key |= static_cast<TSortKey>(column) << 48;
        return key;
    }

    /// The global shader UID
    uint64_t shaderGUID{0};

    /// The internal file UID
    uint16_t fileUID{0};

    /// Line of the mapping
    uint32_t line{0};

    /// Column of the mapping
    uint16_t column{0};

    /// SGUID value
    ShaderSGUID sguid{InvalidShaderSGUID};
};
