#pragma once

// Common
#include "ContainerAllocator.h"

// Std
#include <vector>

/// Container vector
template<typename T>
using Vector = std::vector<T, ContainerAllocator<T>>;
