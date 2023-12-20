#pragma once

// Common
#include <Common/IComponent.h>

class ITestPass : public TComponent<ITestPass> {
public:
    COMPONENT(ITestPass);

    /// Run the test
    /// \return success state
    virtual bool Run() = 0;
};
