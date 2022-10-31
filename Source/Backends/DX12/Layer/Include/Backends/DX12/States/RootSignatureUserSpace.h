#pragma once

// Layer
#include "RootSignatureUserMapping.h"

// Common
#include <Common/Containers/TrivialStackVector.h>

struct RootSignatureUserSpace {
    /// All mappings within this user space
    TrivialStackVector<RootSignatureUserMapping, 64u> mappings;
};
