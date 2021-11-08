#pragma once

// Backend
#include "FeatureHookTable.h"

// Forward declarations
class IMessageStorage;
namespace IL {
    struct Program;
}

class IFeature {
public:
    /// Get the hook table of this feature
    /// \return constructed hook table
    virtual FeatureHookTable GetHookTable() = 0;

    /// Collect all produced messages
    /// \param storage the output storage
    virtual void CollectMessages(IMessageStorage* storage) = 0;

    /// Perform injection into a program
    /// \param program the program to be injected to
    virtual void Inject(IL::Program& program) { /* no injection */ }
};
