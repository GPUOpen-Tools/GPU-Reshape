#pragma once

// Common
#include "ContainerAllocator.h"

// BTree
#include <btree/fc_btree.h>

/// Container vector
template<typename K, typename V>
using BTreeMap = frozenca::BTreeMap<K, V, 64, std::ranges::less, ContainerAllocator>;
