#pragma once

// Transform
#include <Transform/Image/Image.h>

// System
#include <Windows.h>

// Std
#include <cstdint>

namespace Win32 {
    /// Find the first window
    /// \param pid process id of the window
    /// \return nullptr if not found
    HWND FindFirstWindow(uint32_t pid);

    /// Take a screenshot of a window
    /// \param pid picks the first window of a process with the given id
    /// \return empty if failed
    Transform::ImageTensor WindowScreenshot(uint32_t pid);

    /// Take a screenshot of a window
    /// \param hwnd handle of the window
    /// \return empty if failed
    Transform::ImageTensor WindowScreenshot(HWND hwnd);
}
