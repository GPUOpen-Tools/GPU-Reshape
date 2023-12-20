#pragma once

// Automation
#include <Test/Automation/Pass/KeyType.h>

// Std
#include <cstdint>

struct KeyInfo {
    /// Undelrying key type
    KeyType type;

    /// Number of times to repeat
    uint32_t repeatCount{1};

    /// Interval on each repeat (ms)
    uint32_t interval{0};

    /// The simulated press (key up - key down) intermission time (ms)
    uint32_t pressIntermission{100};

    /// The diagnostics identifier of this key
    std::string identifier;

    /// Key payload
    union {
        uint32_t platformVirtual;
        uint32_t unicode;
    };
};
