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

#include <Test/Automation/Win32/Window.h>

// System
#include <dwmapi.h>

HWND Win32::FindFirstWindow(uint32_t pid) {
    HWND hwnd{nullptr};

    for(;;) {
        // Find next window
        if (hwnd = FindWindowEx(nullptr, hwnd, nullptr, nullptr); !hwnd) {
            break;
        }

        // Get PID
        DWORD processID{0};
        GetWindowThreadProcessId(hwnd, &processID);

        // Get window rectangle
        RECT windowRect{};
        GetWindowRect(hwnd, &windowRect);

        // Check for matching PID and a non-zero size window
        if (processID == pid && (windowRect.right - windowRect.left) > 0) {
            return hwnd;
        }
    }

    // Nothing found
    return nullptr;
}

Transform::ImageTensor Win32::WindowScreenshot(uint32_t pid) {
    HWND hwnd = FindFirstWindow(pid);
    if (!hwnd) {
        return {};
    }

    // Hwnd found, screenshot it
    return WindowScreenshot(hwnd);
}

Transform::ImageTensor Win32::WindowScreenshot(HWND hwnd) {
    SetForegroundWindow(hwnd);

    // Get inner client rectangle
    RECT localClientRect;
    GetClientRect(hwnd, &localClientRect);

    // Get top-left coordinate of client area
    POINT origin{0, 0};
    ClientToScreen(hwnd, &origin);

    // Create screen-wise window rectangle
    RECT windowRect;
    windowRect.left = origin.x;
    windowRect.top = origin.y;
    windowRect.right = windowRect.left + (localClientRect.right - localClientRect.left);
    windowRect.bottom = windowRect.top + (localClientRect.bottom - localClientRect.top);

    // Get the current DC
    HDC screenDC = GetDC(nullptr);

    // Create a target DC
    HDC compatibleDC = CreateCompatibleDC(screenDC);

    // Determine rectangle dimensions
    uint32_t width = windowRect.right - windowRect.left;
    uint32_t height = windowRect.bottom - windowRect.top;

    // Target bitmap data (RGB 8-unorm)
    BITMAPINFO bitmapInfo;
    ZeroMemory(&bitmapInfo, sizeof(BITMAPINFO));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -static_cast<int32_t>(height);
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 24;

    // Target bitmap data
    void *bitmapData;

    // Create section with data handle
    HBITMAP bitmap = CreateDIBSection(screenDC, &bitmapInfo, DIB_RGB_COLORS, &bitmapData, nullptr, 0);

    // Select and blit all data
    SelectObject(compatibleDC, bitmap);
    BitBlt(
        compatibleDC,
        0, 0,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        screenDC,
        windowRect.left,
        windowRect.top,
        SRCCOPY
    );

    auto* channelData = static_cast<uint8_t*>(bitmapData);

    // Create final tensor
    Transform::ImageTensor tensor;
    tensor.resize(3, width, height);

    // Process all scanlines
    for (uint32_t y = 0; y < height; y++) {
        // Compute offset from current scanline
        uint32_t byteOffset = y * ((((width * bitmapInfo.bmiHeader.biBitCount) + 31) & ~31) >> 3);

        // Write scanline (BGR -> RGB)
        for (uint32_t x = 0; x < width; x++) {
            tensor(2, x, y) = static_cast<float>(channelData[byteOffset++]) / 255.0f;
            tensor(1, x, y) = static_cast<float>(channelData[byteOffset++]) / 255.0f;
            tensor(0, x, y) = static_cast<float>(channelData[byteOffset++]) / 255.0f;
        }
    }

    // Cleanup
    DeleteDC(compatibleDC);
    ReleaseDC(nullptr, screenDC);
    return tensor;
}
