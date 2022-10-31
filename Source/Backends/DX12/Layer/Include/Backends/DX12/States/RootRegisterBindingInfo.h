#pragma once

// Std
#include <cstdint>

struct RootRegisterBindingInfo {
    /// Space index
    uint32_t space{~0u};

    /// Shader export register
    uint32_t shaderExportBaseRegister;
    uint32_t shaderExportCount;

    /// PRMT register
    uint32_t prmtBaseRegister;

    /// Descriptor constant register
    uint32_t descriptorConstantBaseRegister;

    /// Event constant register
    uint32_t eventConstantBaseRegister;
};
