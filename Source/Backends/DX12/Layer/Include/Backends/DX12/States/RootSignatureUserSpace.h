#pragma once

// Layer
#include "RootSignatureUserMapping.h"

// Common
#include <Common/Containers/TrivialStackVector.h>

struct RootSignatureUserSpace {
    /// All mappings within this user space
    std::vector<RootSignatureUserMapping> mappings;
};
