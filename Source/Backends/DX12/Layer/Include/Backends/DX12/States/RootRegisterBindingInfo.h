#pragma once

// Std
#include <cstdint>

struct RootRegisterBindingInfo {
    /// Space index
    uint32_t space{~0u};

    /// Shader export register
    uint32_t shaderExportBaseRegister;
    uint32_t shaderExportCount;

    /// PRMT registers
    uint32_t resourcePRMTBaseRegister;
    uint32_t samplerPRMTBaseRegister;

    /// Shader data register
    uint32_t shaderDataConstantRegister;

    /// Descriptor constant register
    uint32_t descriptorConstantBaseRegister;

    /// Event constant register
    uint32_t eventConstantBaseRegister;

    /// Shader resource register
    uint32_t shaderResourceBaseRegister;
    uint32_t shaderResourceCount;
};
