#pragma once

// Common
#include "ContainerAllocator.h"

// UnorderedDense
#include <ankerl/unordered_dense.h>

/// Container vector
template<typename K, typename V>
using UnorderedDense = ankerl::unordered_dense::map<K, V, ankerl::unordered_dense::hash<K>, std::equal_to<K>, ContainerAllocator<std::pair<K, V>>>;
