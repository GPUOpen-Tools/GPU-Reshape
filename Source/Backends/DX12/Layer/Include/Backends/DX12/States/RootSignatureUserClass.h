#pragma once

// Layer
#include "RootSignatureUserSpace.h"

// Common
#include <Common/Containers/TrivialStackVector.h>

struct RootSignatureUserClass {
    RootSignatureUserClass(const Allocators& allocators) : spaces(allocators) {
        
    }
    
    /// All user spaces within this class
    Vector<RootSignatureUserSpace> spaces;
};
