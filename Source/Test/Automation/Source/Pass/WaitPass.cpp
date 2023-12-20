#include <Test/Automation/Diagnostic/DiagnosticScope.h>
#include <Test/Automation/Pass/WaitPass.h>

WaitPass::WaitPass(std::chrono::milliseconds duration): duration(duration) {

}

bool WaitPass::Run() {
    DiagnosticScope scope(registry, "Waiting for {0} ms", duration.count());
    std::this_thread::sleep_for(duration);
    return true;
}
