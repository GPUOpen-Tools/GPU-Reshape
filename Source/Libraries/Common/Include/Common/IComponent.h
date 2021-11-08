#pragma once

// Common
#include "CRC.h"
#include "Allocators.h"

// Std
#include <numeric>

// Backwards reference
class Registry;

/// Registry component
class IComponent {
public:
    Allocators allocators{};
    Registry* registry{nullptr};
};

/// Component implementation
#define COMPONENT(CLASS) \
    static constexpr uint32_t kID = std::integral_constant<uint32_t, crcdetail::compute(#CLASS, sizeof(#CLASS)-1)>::value
