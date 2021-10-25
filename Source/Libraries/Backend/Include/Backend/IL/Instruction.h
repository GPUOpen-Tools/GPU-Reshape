#pragma once

// Backend
#include "OpCode.h"
#include "ID.h"

namespace IL {
    struct Instruction {
        OpCode opCode;
    };

    struct LoadInstruction : public Instruction {
        ID address;
        ID result;
    };

    struct StoreInstruction : public Instruction {
        ID address;
        ID value;
    };

    struct LoadTextureInstruction : public Instruction {
        ID texture;
        ID index;
        ID result;
    };
}
