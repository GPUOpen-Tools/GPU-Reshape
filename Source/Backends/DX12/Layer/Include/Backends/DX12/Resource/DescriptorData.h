#pragma once

// Std
#include <cstdint>

/// Invalid offset for resource heaps, this is safe as the prefix will consume at least one descriptor slot
static constexpr uint32_t kDescriptorDataResourceInvalidOffset = 0;

/// Invalid offset for sampler heaps, sampler heaps do not support prefixes and have a hard limit of 2048,
/// so, treat that as invalid.
static constexpr uint32_t kDescriptorDataSamplerInvalidOffset = 2048;
