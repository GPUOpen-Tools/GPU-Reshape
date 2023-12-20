#pragma once

// Automation
#include <Test/Automation/Pass/ITestPass.h>

// Transform
#include <Transform/Image/Image.h>

class ScreenshotPass : public ITestPass {
public:
    COMPONENT(ScreenshotPass);

    /// Constructor
    /// \param quiet if true, only logs on failure
    ScreenshotPass(bool quiet = false);

    /// Overrides
    bool Run() override;

    /// Get the captured image
    const Transform::ImageTensor& GetImage() const {
        return image;
    }

private:
    /// Captured image
    Transform::ImageTensor image;

    /// Quiet logging?
    bool quiet;
};
