#pragma once

// Std
#include <cstdint>

/// Allocation id for a shader export
using ShaderExportID = uint32_t;

/// Shader source guid identifier
using ShaderSGUID = uint32_t;

/// Total amount of bits for the source guid
static constexpr uint32_t kShaderSGUIDBitCount = 16u;
