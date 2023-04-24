#pragma once

// Std
#include <cstdint>

/// Data layout constants
static constexpr uint32_t kDescriptorDataOffsetDWord = 0u;
static constexpr uint32_t kDescriptorDataLengthDWord = 1u;
static constexpr uint32_t kDescriptorDataDWordCount  = 2u;

/// Sanity check
static_assert(kDescriptorDataDWordCount == kDescriptorDataLengthDWord + 1u, "Mismatched descriptor data length");
