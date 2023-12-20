#pragma once

// Automation
#include <Test/Automation/Pass/ITestPass.h>

class WaitForScreenshotPass : public ITestPass {
public:
    COMPONENT(WaitForScreenshotPass);

    /// Constructor
    /// \param path comparison screenshot path
    /// \param threshold average hash distance threshold
    WaitForScreenshotPass(const std::string& path, int64_t threshold);

    /// Overrides
    bool Run() override;

private:
    /// Reference path
    std::string path;

    /// Given threshold
    int64_t threshold;
};
