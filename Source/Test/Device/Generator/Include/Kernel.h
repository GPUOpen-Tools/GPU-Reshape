#pragma once

// Std
#include <string_view>

enum class KernelType {
    None,
    Compute
};

struct Kernel {
    /// Type of this kernel
    KernelType type{KernelType::None};

    /// Name of this kernel
    std::string_view name;
};
