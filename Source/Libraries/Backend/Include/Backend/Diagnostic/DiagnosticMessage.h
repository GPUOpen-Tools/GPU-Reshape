#pragma once

// Std
#include <cstdint>

template<typename T>
struct DiagnosticMessage {
    /// Type of this message
    T type{};

    /// Base argument stack address
    const void* argumentBase{nullptr};

    /// Byte size of the argument stack
    uint32_t argumentSize{0};
};
