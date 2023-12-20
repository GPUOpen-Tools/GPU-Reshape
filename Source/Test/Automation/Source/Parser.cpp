// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

// Test
#include <Test/Automation/Parser.h>
#include <Test/Automation/Pass/ApplicationInfo.h>
#include <Test/Automation/Pass/ApplicationPass.h>
#include <Test/Automation/Pass/ClickPass.h>
#include <Test/Automation/Pass/KeyPass.h>
#include <Test/Automation/Pass/SequencePass.h>
#include <Test/Automation/Pass/WaitForScreenshotPass.h>
#include <Test/Automation/Pass/WaitPass.h>
#include <Test/Automation/Pass/StandardInstrumentPass.h>
#include <Test/Automation/TestContainer.h>
#include <Test/Automation/Diagnostic/Diagnostic.h>

// System
#if _WIN32
#include <Windows.h>
#endif // _WIN32

// Std
#include <filesystem>

Parser::Parser() {
}

bool Parser::Parse(const nlohmann::json &json, ComRef<ITestPass>& out) {
    if (!json.contains("Applications")) {
        Log(registry, "Expected application key");
        return false;
    }

    // All passes
    std::vector<ComRef<ITestPass>> passes;

    // Create application passes
    for (auto&& kv : json["Applications"]) {
        ComRef<ITestPass> pass;

        // Try to parse application pass
        if (!ParseApplication(kv, pass)) {
            return false;
        }

        passes.push_back(pass);
    }

    // Create sequence from all applications
    out = registry->New<SequencePass>(passes, false);
    return true;
}

static std::filesystem::path ExpandPath(const std::filesystem::path &path) {
    std::string str = path.string();

#if _WIN32
    // Determine the expected size
    DWORD size = ExpandEnvironmentStringsA(str.c_str(), nullptr, 0);
    if (size == 0) {
        return path;
    }

    // Preallocate exapnded
    std::string expanded;
    expanded.resize(size - 1u);

    // For +1u, see https://learn.microsoft.com/en-us/windows/win32/api/processenv/nf-processenv-expandenvironmentstringsa
    ExpandEnvironmentStringsA(str.c_str(), expanded.data(), size + 1u);
    expanded.pop_back();

    // OK
    return expanded;
#else // _WIN32
#error Not implemented
#endif // _WIN32
}

bool Parser::ParseApplication(const nlohmann::json &json, ComRef<ITestPass>& out) {
    ApplicationInfo info;

    // Parse attributes
    info.enabled = json.value("Enabled", true);
    info.requiresDiscovery = json.value("Discovery", false);
    info.arguments = json.value("Arguments", "");

    // Determine type
    if (json.contains("Path")) {
        std::filesystem::path path = ExpandPath(json["Path"].get<std::string>());

        // Assume executable
        info.type = ApplicationLaunchType::Executable;
        info.identifier = path.string();
        info.processName = path.filename().string();
    } else if (json.contains("SteamID")) {
        info.type = ApplicationLaunchType::Steam;
        info.identifier = json["SteamID"].get<std::string>();
        info.processName = json.value("Process", "NoName");
    } else {
        Log(registry, "Unknown environment");
        return false;
    }

    // All internal passes
    std::vector<ComRef<ITestPass>> passes;

    // Actual passes are optional
    if (json.contains("Passes")) {
        for (auto&& step : json["Passes"]) {
            // Try to parse pass
            ComRef<ITestPass> stepPass;
            if (!ParsePass(step, stepPass)) {
                return false;
            }

            passes.push_back(stepPass);
        }
    }

    // Always instrument all applications
    passes.push_back(registry->New<StandardInstrumentPass>());

    // Create passes
    out = registry->New<ApplicationPass>(info, registry->New<SequencePass>(passes, true));
    return true;
}

bool Parser::ParsePass(const nlohmann::json& json, ComRef<ITestPass>& out) {
    if (!json.contains("Type")) {
        Log(registry, "Expected type");
        return false;
    }

    // Get type
    std::string type = json["Type"].get<std::string>();

    // Parse based on type
    if (type == "WaitForScreenshot") {
        return ParseWaitForScreenshot(json, out);
    } else if (type == "Key") {
        return ParseKey(json, out);
    } else if (type == "Wait") {
        return ParseWait(json, out);
    } else if (type == "Click") {
        return ParseClick(json, out);
    } else {
        Log(registry, "Unknown step type: '{0}'", type);
        return false;
    }
}

bool Parser::ParseWaitForScreenshot(const nlohmann::json &json, ComRef<ITestPass>& out) {
    if (!json.contains("Path")) {
        Log(registry, "Expected image path");
        return false;
    }

    // Get threshold
    int64_t threshold = json.value("Threshold", 4ll);

    // Negate value / default if requested
    if (json.value("Negated", false)) {
        threshold = -threshold;
    }

    // Create pass
    out = registry->New<WaitForScreenshotPass>(
        ExpandPath(json["Path"].get<std::string>()).string(),
        threshold
    );
    return true;
}

bool Parser::GetKeyFor(std::string_view key, KeyInfo& out) {
    DWORD virtualKey = 0;

    // Check known virtuals
    if (key == "Enter") {
        virtualKey = VK_RETURN;
    } else if (key == "Left") {
        virtualKey = VK_LEFT;
    } else if (key == "Right") {
        virtualKey = VK_RIGHT;
    } else if (key == "Up") {
         virtualKey = VK_UP;
    } else if (key == "Down") {
        virtualKey = VK_DOWN;
    } else {
        // Not known, assume single character
        if (key.length() != 1) {
            Log(registry, "Invalid key, must be known virtual or one character");
            return false;
        }

        // Try to map it directly to a virtual key
        virtualKey = VkKeyScanEx(key[0], GetKeyboardLayout(0)) & 0xFF;
    }

    // If the mapping succeeded
    if (virtualKey && virtualKey != 0xFF) {
        out.type = KeyType::PlatformVirtual;
        out.platformVirtual = virtualKey;
    } else {
        // Failed, just assume unicode
        out.type = KeyType::Unicode;
        out.platformVirtual = static_cast<uint32_t>(key[0]);
    }

    // OK
    return true;
}

bool Parser::ParseKey(const nlohmann::json &json, ComRef<ITestPass>& out) {
    if (!json.contains("Key")) {
        Log(registry, "Expected key");
        return false;
    }

    KeyInfo info;

    // Get the actual key info
    if (!GetKeyFor(json["Key"].get<std::string>(), info)) {
        return false;
    }

    // Parse additional attributes
    info.interval = json.value("Interval", 250u);
    info.repeatCount = json.value("Repeat", 1u);
    info.identifier = json["Key"];

    // Create pass
    out = registry->New<KeyPass>(info);
    return true;
}

bool Parser::ParseWait(const nlohmann::json &json, ComRef<ITestPass>& out) {
    out = registry->New<WaitPass>(std::chrono::milliseconds(json.value("Duration", 1000)));
    return true;
}

bool Parser::ParseClick(const nlohmann::json &json, ComRef<ITestPass>& out) {
    out = registry->New<ClickPass>(
        json.value("X", 0.0f),
        json.value("Y", 0.0f),
        json.value("Normalized", false)
    );
    return true;
}
