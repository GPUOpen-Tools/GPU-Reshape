#pragma once

// Std
#include <cstdint>

/// Allocation id for shader data
using ShaderDataID = uint32_t;

/// Invalid values
static constexpr ShaderDataID InvalidShaderDataID = ~0u;
