#pragma once

// Layer
#include "LLVMHeader.h"

// Common
#include <Common/Containers/TrivialStackVector.h>

/// LLVM Record Abbreviation
struct LLVMAbbreviation {
    /// All parameters of this abbreviation for later expansion
    TrivialStackVector<LLVMAbbreviationParameter, 32> parameters;
};
