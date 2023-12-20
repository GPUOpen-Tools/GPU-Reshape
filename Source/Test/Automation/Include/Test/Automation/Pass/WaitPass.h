#pragma once

// Automation
#include <Test/Automation/Pass/ITestPass.h>

// Std
#include <chrono>

class WaitPass : public ITestPass {
public:
    COMPONENT(WaitPass);

    /// Constructor
    /// \param duration number of milliseconds to wait for
    WaitPass(std::chrono::milliseconds duration);

    /// Overrides
    bool Run() override;

private:
    /// Wait duration
    std::chrono::milliseconds duration;
};
