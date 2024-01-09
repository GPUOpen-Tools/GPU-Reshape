// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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
