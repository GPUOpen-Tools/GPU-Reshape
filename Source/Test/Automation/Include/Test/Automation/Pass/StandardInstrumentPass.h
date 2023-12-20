#pragma once

// Automation
#include <Test/Automation/Pass/ITestPass.h>

class StandardInstrumentPass : public ITestPass {
public:
    COMPONENT(ClickPass);

    /// Overrides
    bool Run() override;
};
