#pragma once

// Std
#include <cstdint>

using ShaderProgramID = uint32_t;

/// Invalid program handle
static constexpr ShaderProgramID InvalidShaderProgramID = ~0u;
