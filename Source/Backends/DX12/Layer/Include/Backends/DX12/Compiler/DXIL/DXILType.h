#pragma once

// Backend
#include <Backend/IL/Type.h>

// Layer
#include "LLVM/LLVMHeader.h"

struct DXILType : public Backend::IL::UnexposedType {
    LLVMTypeRecord type;
};
