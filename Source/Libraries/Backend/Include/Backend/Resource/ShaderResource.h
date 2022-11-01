#pragma once

// Std
#include <cstdint>

/// Allocation id for a shader resource
using ShaderResourceID = uint32_t;

/// Invalid values
static constexpr ShaderResourceID InvalidShaderResourceID = ~0u;
