#pragma once

// Common
#include <Common/IComponent.h>

// Forward declarations
class IShaderDataHost;

// IL
namespace IL {
    struct Program;
}

class IShaderProgram : public TComponent<IShaderProgram> {
public:
    COMPONENT(IShaderProgram);

    /// Perform injection into a program
    /// \param program the program to be injected to
    virtual void Inject(IL::Program& program) = 0;
};
