#pragma once

// Layer
#include "ReconstructionFlag.h"

// Backend
#include <Backend/ShaderProgram/ShaderProgram.h>

struct UserCommandState {
    /// Current reconstruction state
    ReconstructionFlagSet reconstructionFlags{0};

    /// Bound shader program
    ShaderProgramID shaderProgramID;
};
