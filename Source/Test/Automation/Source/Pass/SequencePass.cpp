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

#include <Test/Automation/Pass/SequencePass.h>
#include <Test/Automation/Pass/ScreenshotPass.h>
#include <Test/Automation/Diagnostic/Diagnostic.h>
#include <Test/Automation/Data/ApplicationData.h>

// Transform
#include <Transform/Image/IO.h>

// Common
#include <Common/FileSystem.h>

SequencePass::SequencePass(const std::vector<ComRef<ITestPass>>& passes, bool stronglyChained): passes(passes), stronglyChained(stronglyChained) {

}

bool SequencePass::Run() {
    // Create screenshot pass
    ComRef screenshot = registry->New<ScreenshotPass>(true);

    // Local UID to this pass invocation
    uint64_t counter = registry->Get<Diagnostic>()->AllocateCounter();

    // Optional application data
    ComRef data = registry->Get<ApplicationData>();

    // Failure state
    bool result = true;

    // RUn all passes
    for (size_t i = 0; i < passes.size(); i++) {
        ComRef pass = passes[i];

        // Application specific
        if (data) {
            // Check if's still running
            if (!data->IsAlive()) {
                Log(registry, "Application lost");
                return false;
            }

            // Take a screenshot inbetween passes, useful for debugging failed applications
            if (screenshot->Run()) {
                std::filesystem::path path = GetIntermediatePath("Automation");
                Transform::WriteImage(
                    Format("{0}/Seq{1}_{2}_{3}.png", path.string(), counter, i, pass->componentName.name).c_str(),
                    screenshot->GetImage()
                );
            }
        }

        // Try to run
        if (!pass->Run()) {
            if (stronglyChained) {
                return false;
            }

            // Weak chaining
            result = false;
        }
    }

    return result;
}
