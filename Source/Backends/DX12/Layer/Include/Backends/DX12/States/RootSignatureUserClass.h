#pragma once

// Layer
#include "RootSignatureUserSpace.h"

// Common
#include <Common/Containers/TrivialStackVector.h>

struct RootSignatureUserClass {
    /// All user spaces within this class
    std::vector<RootSignatureUserSpace> spaces;
};
