// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

#include <Test/Automation/Pass/ClickPass.h>
#include <Test/Automation/Diagnostic/DiagnosticScope.h>
#include <Test/Automation/Win32/Window.h>
#include <Test/Automation/Data/ApplicationData.h>

ClickPass::ClickPass(float localX, float localY, bool normalized): localX(localX), localY(localY), normalized(normalized) {

}

bool ClickPass::Run() {
    DiagnosticScope scope(registry, "Clicking at [{0}, {1}]", localX, localY);

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

    // Input simulation is global, bring the window in focus
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    // Get window rectangle
    RECT windowRect{};
    GetWindowRect(hwnd, &windowRect);

    uint32_t realLocalX;
    uint32_t realLocalY;

    // Normalize to client
    if (normalized) {
        realLocalX = static_cast<uint32_t>(localX * (windowRect.right - windowRect.left));
        realLocalY = static_cast<uint32_t>(localY * (windowRect.bottom - windowRect.top));
    } else {
        realLocalX = static_cast<uint32_t>(localX);
        realLocalY = static_cast<uint32_t>(localY);
    }

    // Absolute coordinates
    uint32_t screenX = windowRect.left + realLocalX;
    uint32_t screenY = windowRect.top + realLocalY;

    // Screen dimensions
    uint32_t screenWidth = GetSystemMetrics(SM_CXSCREEN);
    uint32_t screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create input events
    // Note that absolute dx / dy coordinates are normalized to 2^16
    INPUT inputs[] = {
        {
            .type = INPUT_MOUSE,
            .mi = {
                .dx = static_cast<LONG>(static_cast<float>(screenX) * (65535 / (float)screenWidth)),
                .dy = static_cast<LONG>(static_cast<float>(screenY) * (65535 / (float)screenHeight)),
                .dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE
            }
        },
        {
            .type = INPUT_MOUSE,
            .mi = {
                .dwFlags = MOUSEEVENTF_LEFTDOWN
            }
        },
        {
            .type = INPUT_MOUSE,
            .mi = {
                .dwFlags = MOUSEEVENTF_LEFTUP
            }
        }
    };

    // Send input
    if (SendInput(3u, inputs, sizeof(INPUT)) != 3u) {
        Log(registry, "Failed to send click events");
        return false;
    }
#else // _WIN32
#error Not implemented
#endif // _WIN32

    // OK
    return true;
}
