#pragma once

// Layer
#include "RootSignatureUserClass.h"
#include "RootSignatureUserClassType.h"

struct RootSignatureVisibilityClass {    
    /// All register binding classes
    RootSignatureUserClass spaces[static_cast<uint32_t>(RootSignatureUserClassType::Count)];
};
