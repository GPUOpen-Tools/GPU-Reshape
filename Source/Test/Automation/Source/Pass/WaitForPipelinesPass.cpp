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

#include <Test/Automation/Pass/WaitForPipelinesPass.h>
#include <Test/Automation/Diagnostic/DiagnosticScope.h>
#include <Test/Automation/Connection/Connection.h>
#include <Test/Automation/Data/ApplicationData.h>

// Schemas
#include <Schemas/Object.h>

bool WaitForPipelinesPass::Run() {
    DiagnosticScope scope(registry, "Waiting for pipelines to stabilize...");

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

    // Pool object count until stable
    for (uint64_t lastPipelineCount = UINT64_MAX;;) {
        PooledMessage objects = connection->Pool<ObjectStatesMessage>(PoolingMode::Replace);
        connection->AddAndCommit<GetObjectStatesMessage>();

        // Wait for objects
        while (!objects.Wait(std::chrono::seconds(5))) {
            // Check if the application didn't crash somewhere
            if (!data->IsAlive()) {
                Log(registry, "Application crashed during tests");
                return false;
            }
        }

        // Stable?
        if (lastPipelineCount == objects->pipelineCount) {
            Log(registry, "Pipelines stabilized");
            break;
        }

        // Not stable
        lastPipelineCount = objects->pipelineCount;
        Log(registry, "Pipeline count: {0}", lastPipelineCount);

        // Wait a bit
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    // OK
    return true;
}
