#include <Test/Automation/Pass/ScreenshotPass.h>
#include <Test/Automation/Win32/Window.h>
#include <Test/Automation/Data/ApplicationData.h>
#include <Test/Automation/Diagnostic/DiagnosticScope.h>

// Common
#include <Common/Registry.h>

ScreenshotPass::ScreenshotPass(bool quiet): quiet(quiet) {

}

bool ScreenshotPass::Run() {
    DiagnosticScope scope(registry, !quiet, "Screenshot");

    // Get app data
    ComRef data = registry->Get<ApplicationData>();
    if (!data) {
        Log(registry, "Missing application data");
        return false;
    }

#if _WIN32
    // Try to find active window
    HWND hwnd = Win32::FindFirstWindow(data->processID);
    if (!hwnd) {
        Log(registry, "Failed to find window");
        return false;
    }

    // Get window rectangle
    RECT windowRect{};
    GetWindowRect(hwnd, &windowRect);

    // Highly arbritrary threshold
    if (windowRect.right - windowRect.left < 25) {
        return {};
    }

    // Take screenshot
    image = Win32::WindowScreenshot(hwnd);
#else // _WIN32
#error Not implemented
#endif // _WIN32

    // Did the image pass?
    return image.size() != 0;
}
