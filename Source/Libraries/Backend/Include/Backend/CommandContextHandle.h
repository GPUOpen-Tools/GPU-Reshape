#pragma once

// Std
#include <cstdint>

/// User context handle
using CommandContextHandle = uint64_t;

/// Invalid handle
static constexpr CommandContextHandle kInvalidCommandContextHandle = ~0ull;
