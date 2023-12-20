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
