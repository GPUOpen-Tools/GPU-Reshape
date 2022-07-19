#pragma once

// Layer
#include <Backends/DX12/Compiler/DXStream.h>

// Std
#include <cstdint>

struct DXBCPhysicalBlock {
    /// Block starting address
    const uint8_t* ptr{nullptr};

    /// Size of this block
    uint32_t length{0};

    /// Final stream
    DXStream stream;
};
