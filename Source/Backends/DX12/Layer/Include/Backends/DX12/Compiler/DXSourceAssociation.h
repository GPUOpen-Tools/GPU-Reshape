#pragma once

// Std
#include <cstdint>

/// Source association
struct DXSourceAssociation {
    operator bool() const {
        return fileUID != UINT16_MAX;
    }

    uint16_t fileUID{UINT16_MAX};
    uint32_t line{0};
    uint16_t column{0};
};
