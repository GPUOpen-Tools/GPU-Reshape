#include <Test/Automation/Pass/KeyPass.h>
#include <Test/Automation/Diagnostic/DiagnosticScope.h>
#include <Test/Automation/Win32/Window.h>
#include <Test/Automation/Data/ApplicationData.h>

KeyPass::KeyPass(const KeyInfo& info): info(info) {

}

bool KeyPass::Run() {
    DiagnosticScope scope(registry, "Key {0}", info.identifier);

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

    // Create input event
    INPUT input{};
    switch (info.type) {
        default: {
            ASSERT(false, "Invalid type");
            break;
        }
        case KeyType::PlatformVirtual: {
            input = {
                .type = INPUT_KEYBOARD,
                .ki = {
                    .wVk = static_cast<WORD>(info.platformVirtual),
                    .wScan = static_cast<WORD>(MapVirtualKey(info.platformVirtual, MAPVK_VK_TO_VSC)),
                    .dwFlags = KEYEVENTF_SCANCODE
                }
            };

            if (IsExtendedKey(info.platformVirtual)) {
                input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
            }
            break;
        }
        case KeyType::Unicode: {
            input = {
                .type = INPUT_KEYBOARD,
                .ki = {
                    .wScan = static_cast<WORD>(info.unicode),
                    .dwFlags = KEYEVENTF_UNICODE
                }
            };
            break;
        }
    }

    // Handle repeats
    for (uint32_t i = 0; i < info.repeatCount; i++) {
        INPUT localInput = input;

        // Diagnostic
        if (info.repeatCount > 1) {
            Log(registry, "Repeat [{0} / {1}]", i, info.repeatCount);
        }

        // Send key down
        if (SendInput(1u, &localInput, sizeof(INPUT)) != 1u) {
            Log(registry, "Failed to send input");
            return false;
        }

        // Simulate key intermission
        // Certain games seem to rely on an actual measurable intermission, instead of the events.
        // Of course this is not infallable, if the game simulation step is longer than the intermission,
        // nothing will be "recorded".
        std::this_thread::sleep_for(std::chrono::milliseconds(info.pressIntermission));

        // Assume key up
        localInput.ki.dwFlags |= KEYEVENTF_KEYUP;

        // Send key up
        if (SendInput(1u, &localInput, sizeof(INPUT)) != 1u) {
            Log(registry, "Failed to send input");
            return false;
        }

        // Wait for interval
        std::this_thread::sleep_for(std::chrono::milliseconds(info.interval));
    }
#else // _WIN32
#error Not implemented
#endif // _WIN32

    return true;
}

#if _WIN32
bool KeyPass::IsExtendedKey(uint32_t virtualKey) {
    // Note: This is far from the full list of extended keys, but will suffice for now
    switch (virtualKey) {
        default:
            return false;
        case VK_INSERT:
        case VK_DELETE:
        case VK_HOME:
        case VK_END:
        case VK_RIGHT:
        case VK_LEFT:
        case VK_UP:
        case VK_DOWN:
        case VK_APPS:
        case VK_LWIN:
        case VK_RWIN:
            return true;
    }
}
#endif // _WIN32
