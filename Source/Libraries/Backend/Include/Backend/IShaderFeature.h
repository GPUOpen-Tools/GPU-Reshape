#pragma once

// Backend
#include "FeatureHookTable.h"

// Common
#include <Common/IComponent.h>
#include <Message/MessageStream.h>

// Forward declarations
struct MessageStream;

// IL
namespace IL {
    struct Program;
}

class IShaderFeature : public IInterface {
public:
    COMPONENT(IShaderFeature);

    /// Collect all shader exports
    /// \param stream the produced stream
    virtual void CollectExports(const MessageStream& stream) { /* no collection */};

    /// Perform injection into a program
    /// \param program the program to be injected to
    virtual void Inject(IL::Program &program, const MessageStreamView<> &specialization) { /* no injection */ }
};
