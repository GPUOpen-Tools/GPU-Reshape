#pragma once

// Automation
#include <Test/Automation/Pass/ITestPass.h>

// Common
#include <Common/ComRef.h>

// Std
#include <vector>

class SequencePass : public ITestPass {
public:
    COMPONENT(SequencePass);

    /// Constructor
    /// \param passes all passes to execute in sequence
    /// \param stronglyChained if true, a pass failing will stop all passes
    SequencePass(const std::vector<ComRef<ITestPass>>& passes, bool stronglyChained);

    /// Overrides
    bool Run() override;

private:
    /// All passes
    std::vector<ComRef<ITestPass>> passes;

    /// One test stops all other tests
    bool stronglyChained;
};
