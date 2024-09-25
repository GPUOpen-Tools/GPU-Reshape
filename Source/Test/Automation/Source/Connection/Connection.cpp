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

#include <Test/Automation/Connection/Connection.h>
#include <Test/Automation/Connection/PoolingListener.h>

// Bridge
#include <Bridge/IBridge.h>
#include <Bridge/RemoteClientBridge.h>

// Schema
#include <Schemas/Feature.h>
#include <Schemas/Instrumentation.h>

// Common
#include <Common/Registry.h>

Connection::Connection(): view(stream) {
    stream.SetSchema(MessageSchema {
        .type = MessageSchemaType::Ordered,
        .id = InvalidMessageID
    });
}

Connection::~Connection() {
    // Cleanup bridge
    bridge->Deregister(listener);
}

bool Connection::Install(const EndpointResolve& resolve, const std::string_view& processName) {
    // Create bridge
    bridge = registry->New<RemoteClientBridge>();
    bridge->SetCommitOnAppend(true);

    // Try to install, synchronous
    if (!bridge->Install(resolve)) {
        return false;
    }

    // Attach listener
    listener = registry->New<PoolingListener>();
    bridge->Register(listener);

    // Try to create a connection
    if (!CreateConnection(processName)) {
        return false;
    }

    // Pool all features
    if (!PoolFeatures()) {
        return false;
    }

    // OK
    return true;
}

PooledMessage<InstrumentationDiagnosticMessage> Connection::InstrumentGlobal(const InstrumentationConfig& config) {
    auto* global = view.Add<SetGlobalInstrumentationMessage>();
    global->featureBitSet = config.featureBitSet;
    Commit();

    // Pool the result
    return Pool<InstrumentationDiagnosticMessage>();
}

bool Connection::CreateConnection(const std::string_view& processName) {
    PooledMessage discovery = Pool<HostDiscoveryMessage>(PoolingMode::Replace);
    PooledMessage accepted  = Pool<HostConnectedMessage>();

    // Starting time stamp
    auto stamp = std::chrono::high_resolution_clock::now();

    // Keep pooling until a connection is requested or time threshold is reached
    for (;;) {
        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - stamp).count() >= 180) {
            return false;
        }

        // Request discovery
        bridge->DiscoverAsync();

        // Generic wait
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        // Pool all host servers from discovery
        if (PoolHostServers(processName, discovery)) {
            break;
        }
    }

    // A connection was made, pool for success
    return accepted->accepted;
}

bool Connection::PoolHostServers(const std::string_view& processName, PooledMessage<HostDiscoveryMessage>& pool) {
    while (const HostDiscoveryMessage* discovery = pool.Pool(true)) {
        // Transfer all instances
        MessageStream stream;
        discovery->infos.Transfer(stream);

        // Visit all instances
        for (auto it = MessageStreamView(stream).GetIterator(); it; ++it) {
            auto* info = it.Get<HostServerInfoMessage>();

            // Matches process?
            if (processName != info->process.View() || info->deviceObjects < objectThreshold) {
                continue;
            }

            // Assume PID
            processID = static_cast<uint32_t>(info->processId);

            // Request a connection to the server
            bridge->RequestClientAsync(GlobalUID::FromString(info->guid.View()));
            return true;
        }
    }

    // No connection made
    return false;
}

bool Connection::PoolFeatures() {
    PooledMessage features = Pool<FeatureListMessage>();

    // Request features
    view.Add<GetFeaturesMessage>();
    Commit();

    // Read all features
    MessageStream featureStream;
    features->descriptors.Transfer(featureStream);

    // Create mappings
    for (auto it = MessageStreamView(featureStream).GetIterator(); it; ++it) {
        if (!it.Is(FeatureDescriptorMessage::kID)) {
            continue;
        }

        auto* descriptor = it.Get<FeatureDescriptorMessage>();
        featureMap[std::string(descriptor->name.View())] = descriptor->featureBit;
    }

    // OK
    return true;
}

void Connection::Commit() {
    bridge->GetOutput()->AddStreamAndSwap(stream);
    bridge->Commit();
}
