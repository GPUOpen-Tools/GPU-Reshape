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

#include <Test/Automation/Pass/WaitForScreenshotPass.h>
#include <Test/Automation/Pass/ScreenshotPass.h>
#include <Test/Automation/Diagnostic/Diagnostic.h>
#include <Test/Automation/Diagnostic/DiagnosticScope.h>
#include <Test/Automation/Data/ApplicationData.h>

// Transform
#include <Transform/Hamming.h>
#include <Transform/Image/IO.h>
#include <Transform/Image/Hash.h>

// Common
#include <Common/ComRef.h>
#include <Common/Registry.h>

WaitForScreenshotPass::WaitForScreenshotPass(const std::string& path, int64_t threshold): path(path), threshold(threshold) {

}

bool WaitForScreenshotPass::Run() {
    // Read the reference image
    Transform::ImageTensor reference = Transform::ReadImage(path.c_str());
    if (!reference.size()) {
        Log(registry, "Failed to load {0}", path);
        return false;
    }

    // Compute hash on reference
    const uint64_t referenceHash = Transform::AverageHash(reference);

    // Create screenshot pass
    ComRef screenshot = registry->New<ScreenshotPass>(true);

    // Local scope
    DiagnosticScope scope(registry, "Waiting for screenshot {0}", path);

    // Get app data
    ComRef data = registry->Get<ApplicationData>();
    if (!data) {
        Log(registry, "Missing application data");
        return false;
    }

    // Wait pass
    for (;;) {
        // Check if the application is still running
        if (!data->IsAlive()) {
            Log(registry, "Application lost");
            return false;
        }

        // Take screenshot
        if (!screenshot->Run()) {
            // Something went wrong, window may not have been created yet, or is being recreated
            // Wait a little and try again
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        // Compute hash
        uint64_t screenshotHash = Transform::AverageHash(screenshot->GetImage());

        // Compute distance between reference and screenshot
        uint64_t distance = Transform::HammingDistance(referenceHash, screenshotHash);

        // Thresholds may be negatie, in which case it's checking for divergence
        bool pass;
        if (threshold >= 0) {
            pass = distance <= static_cast<uint64_t>(threshold);
        } else {
            pass = distance >= static_cast<uint64_t>(-threshold);
        }

        Log(registry, "Hash distance: {0} ({1})", distance, threshold);

        // Passed?
        if (pass) {
            return true;
        }

        // Didn't pass, wait for a little bit
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // OK
    return false;
}
