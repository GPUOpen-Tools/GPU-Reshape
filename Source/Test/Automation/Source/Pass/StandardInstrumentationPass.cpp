#include <Test/Automation/Pass/StandardInstrumentPass.h>
#include <Test/Automation/Diagnostic/DiagnosticScope.h>
#include <Test/Automation/Connection/Connection.h>
#include <Test/Automation/Connection/InstrumentationConfig.h>
#include <Test/Automation/Data/ApplicationData.h>

bool StandardInstrumentPass::Run() {
    DiagnosticScope scope(registry, "Standard instrumentation");

    // Get connection
    ComRef connection = registry->Get<Connection>();
    if (!connection) {
        Log(registry, "Failed to the connection");
        return false;
    }

    // Get app data
    ComRef data = registry->Get<ApplicationData>();
    if (!data) {
        Log(registry, "Missing application data");
        return false;
    }

    // Setup instrumentation bits
    InstrumentationConfig config;
    config.featureBitSet |= connection->GetFeatureBit("Resource Bounds");
    config.featureBitSet |= connection->GetFeatureBit("Export Stability");
    config.featureBitSet |= connection->GetFeatureBit("Initialization");
    config.featureBitSet |= connection->GetFeatureBit("Descriptor");
    config.featureBitSet |= connection->GetFeatureBit("Concurrency");

    // Pool all job diagnostics
    PooledMessage jobs = connection->Pool<JobDiagnosticMessage>(PoolingMode::Replace);

    // Run instrumentation
    PooledMessage report = connection->InstrumentGlobal(config);

    // While the report isn't done
    while (!report) {
        // Check if the application is still running
        if (!data->IsAlive()) {
            Log(registry, "Application lost");
            return false;
        }

        // Pool any job diagnostics
        if (const JobDiagnosticMessage* diagnostic = jobs.Pool(true)) {
            const size_t total = diagnostic->graphicsJobs + diagnostic->computeJobs;
            Log(registry, "[{0}] {1} / {2}", diagnostic->stage == 1u ? "Shaders" : "Pipelines", diagnostic->remaining, total);
        }

        // Wait a little
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Report findings
    DiagnosticScope reportScope(registry, "Instrumentation took {0} ms", report->millisecondsTotal);
    Log(registry, "Shaders, P: {0}, F: {1}, {2} ms", report->passedShaders, report->failedShaders, report->millisecondsShaders);
    Log(registry, "Pipelines, P: {0}, F: {1}, {2} ms", report->passedPipelines, report->failedPipelines, report->millisecondsPipelines);

    // Report failed objects
    if (report->failedPipelines || report->failedShaders) {
        Log(registry, "One or more objects failed");
    }

    // OK
    return true;
}
