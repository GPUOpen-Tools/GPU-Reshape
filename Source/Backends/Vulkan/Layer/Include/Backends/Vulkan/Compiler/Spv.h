#pragma once

// SPIRV
#define SPV_ENABLE_UTILITY_CODE
#include <spirv/unified1/spirv.h>

/// Invalid spirv identifier
static constexpr SpvId InvalidSpvId = ~0u;
