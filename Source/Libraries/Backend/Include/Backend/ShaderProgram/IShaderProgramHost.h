#pragma once

// Backend
#include "ShaderProgram.h"

// Common
#include <Common/ComRef.h>

// Forward declarations
class IShaderProgram;

class IShaderProgramHost : public TComponent<IShaderProgramHost> {
public:
    COMPONENT(IShaderProgramHost);

    /// Register a program
    /// \param program
    virtual ShaderProgramID Register(const ComRef<IShaderProgram>& program) = 0;

    /// Deregister a program
    /// \param program
    virtual void Deregister(ShaderProgramID program) = 0;
};
