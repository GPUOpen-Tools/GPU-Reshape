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

    // Additional features for coverage
    config.featureBitSet |= connection->GetFeatureBit("Waterfall");

    // Enable all optionals
    config.detailed = true;
    config.safeGuarded = true;
    
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
        std::this_thread::sleep_for(std::chrono::seconds(5));
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
