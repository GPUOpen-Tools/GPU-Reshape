#pragma once

// Automation
#include <Test/Automation/Pass/ITestPass.h>
#include <Test/Automation/Pass/KeyInfo.h>

class KeyPass : public ITestPass {
public:
    COMPONENT(KeyPass);

    /// Constructor
    KeyPass(const KeyInfo& info);

    /// Overrides
    bool Run() override;

private:
    /// Check if a key is extended
#if _WIN32
    static bool IsExtendedKey(uint32_t virtualKey);
#endif // _WIN32

private:
    /// Given info
    KeyInfo info;
};
