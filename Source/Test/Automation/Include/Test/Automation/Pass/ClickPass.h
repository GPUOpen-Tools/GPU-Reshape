#pragma once

// Automation
#include <Test/Automation/Pass/ITestPass.h>

class ClickPass : public ITestPass {
public:
    COMPONENT(ClickPass);

    /// Constructor
    /// \param localX relative (to window) x coordinate of click
    /// \param localY relative (to window) y coordinate of click
    /// \param normalized if true, coordinates are 0-1 normalized values
    ClickPass(float localX, float localY, bool normalized);

    /// Overrides
    bool Run() override;

private:
    /// Relative coordinates
    float localX;
    float localY;
    bool normalized;
};
