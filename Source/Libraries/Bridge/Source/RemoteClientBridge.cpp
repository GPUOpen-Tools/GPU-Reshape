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

#include <Bridge/RemoteClientBridge.h>
#include <Bridge/IBridgeListener.h>
#include <Bridge/EndpointConfig.h>
#include <Bridge/NetworkProtocol.h>
#include <Bridge/Asio/AsioRemoteClient.h>

// Message
#include <Message/MessageStream.h>

// Schemas
#include <Schemas/HostResolve.h>


RemoteClientBridge::RemoteClientBridge() {
    // Create the client
    client = new(allocators) AsioRemoteClient();
    
    // Subscribe
    client->onConnected.Add(0, [this](const AsioHostResolverClientRequest::ServerResponse& response) { OnConnected(response); });
    client->onResolve.Add(0, [this](const AsioHostResolverClientRequest::ResolveResponse& response) { OnResolve(response); });
    client->onDiscovery.Add(0, [this](const AsioRemoteServerResolverDiscoveryRequest::Response &response) { OnDiscovery(response); });

    // Set read callback
    client->SetServerReadCallback([this](AsioSocketHandler &handler, const void *data, uint64_t size) {
        return OnReadAsync(data, size);
    });
}

RemoteClientBridge::~RemoteClientBridge() {
    // Release endpoint
    destroy(client, allocators);
}

bool RemoteClientBridge::Install(const EndpointResolve &resolve) {
    // Port config
    AsioRemoteConfig asioConfig;
    asioConfig.hostResolvePort = resolve.config.sharedPort;
    asioConfig.ipvxAddress = resolve.ipvxAddress;
    
    // Try to connect
    return client->Connect(asioConfig);
}

void RemoteClientBridge::InstallAsync(const EndpointResolve &resolve) {
    // Port config
    AsioRemoteConfig asioConfig;
    asioConfig.hostResolvePort = resolve.config.sharedPort;
    asioConfig.ipvxAddress = resolve.ipvxAddress;

    // Try to connect
    client->ConnectAsync(asioConfig);
}

void RemoteClientBridge::Cancel() {
    client->Cancel();
}

void RemoteClientBridge::Stop() {
    client->Stop();
}

void RemoteClientBridge::SetAsyncConnectedDelegate(const AsioClientAsyncConnectedDelegate &delegate) {
    client->SetAsyncConnectedCallback(delegate);
}

void RemoteClientBridge::DiscoverAsync() {
    client->DiscoverAsync();
}

void RemoteClientBridge::RequestClientAsync(const AsioHostClientToken &guid) {
    client->RequestClientAsync(AsioHostClientToken(guid));
}

void RemoteClientBridge::OnConnected(const AsioHostResolverClientRequest::ServerResponse& response) {
    MessageStream stream;

    MessageStreamView view(stream);

    // Push message
    auto* message = view.Add<HostConnectedMessage>();
    message->accepted = response.accepted;

    memoryBridge.GetOutput()->AddStream(stream);

    // Commit all inbound streams if requested
    if (commitOnAppend) {
        memoryBridge.Commit();
    }
}

void RemoteClientBridge::OnResolve(const AsioHostResolverClientRequest::ResolveResponse &response) {
    MessageStream stream;

    MessageStreamView view(stream);

    // Push message
    auto* message = view.Add<HostResolvedMessage>();
    message->accepted = response.found;

    memoryBridge.GetOutput()->AddStream(stream);

    // Commit all inbound streams if requested
    if (commitOnAppend) {
        memoryBridge.Commit();
    }
}

void RemoteClientBridge::OnDiscovery(const AsioRemoteServerResolverDiscoveryRequest::Response &response) {
    // Entry stream
    MessageStream entries;
    {
        MessageStreamView view(entries);

        // Process all entries
        for (uint32_t i = 0; i < response.entryCount; i++) {
            const AsioRemoteServerResolverDiscoveryRequest::Entry &entry = response.entries[i];

            // Convert guids to string
            std::string guid = entry.token.ToString();
            std::string reservedGuid = entry.reservedToken.ToString();

            // Allocate info
            auto *info = view.Add<HostServerInfoMessage>(HostServerInfoMessage::AllocationInfo{
                .guidLength = guid.length(),
                .reservedGuidLength = reservedGuid.length(),
                .processLength = std::strlen(entry.info.processName),
                .applicationLength = std::strlen(entry.info.applicationName),
                .apiLength = std::strlen(entry.info.apiName)
            });

            // Set data
            info->guid.Set(guid);
            info->reservedGuid.Set(reservedGuid);
            info->process.Set(entry.info.processName);
            info->application.Set(entry.info.applicationName);
            info->api.Set(entry.info.apiName);
            info->processId = entry.info.processId;
            info->deviceUid = entry.info.deviceUid;
            info->deviceObjects = entry.info.deviceObjects;
        }
    }

    // Base stream
    MessageStream stream;
    {
        MessageStreamView view(stream);

        // Allocate message
        auto *discovery = view.Add<HostDiscoveryMessage>(HostDiscoveryMessage::AllocationInfo{
            .infosByteSize = entries.GetByteSize()
        });

        // Set entries
        discovery->infos.Set(entries);
    }

    memoryBridge.GetOutput()->AddStream(stream);

    // Commit all inbound streams if requested
    if (commitOnAppend) {
        memoryBridge.Commit();
    }
}

uint64_t RemoteClientBridge::OnReadAsync(const void *data, uint64_t size) {
    auto *protocol = static_cast<const MessageStreamHeaderProtocol *>(data);

    // Entire stream present?
    if (size < sizeof(MessageStreamHeaderProtocol) ||
        size < sizeof(MessageStreamHeaderProtocol) + protocol->size) {
        return 0;
    }

    // Validate header
    ASSERT(protocol->magic == MessageStreamHeaderProtocol::kMagic, "Unexpected magic header");

    // Create the stream
    MessageStream stream(protocol->schema);
    stream.SetVersionID(protocol->versionID);
    stream.SetData(static_cast<const uint8_t *>(data) + sizeof(MessageStreamHeaderProtocol), protocol->size, 0);
    memoryBridge.GetOutput()->AddStream(stream);

    // Commit all inbound streams if requested
    if (commitOnAppend) {
        memoryBridge.Commit();
    }

    // Determine byte count
    const size_t bytes = sizeof(MessageStreamHeaderProtocol) + protocol->size;
    info.bytesRead += bytes;

    // Consume entire stream
    return bytes;
}

void RemoteClientBridge::Register(MessageID mid, const ComRef<IBridgeListener> &listener) {
    memoryBridge.Register(mid, listener);
}

void RemoteClientBridge::Deregister(MessageID mid, const ComRef<IBridgeListener> &listener) {
    memoryBridge.Deregister(mid, listener);
}

void RemoteClientBridge::Register(const ComRef<IBridgeListener> &listener) {
    memoryBridge.Register(listener);
}

void RemoteClientBridge::Deregister(const ComRef<IBridgeListener> &listener) {
    memoryBridge.Deregister(listener);
}

IMessageStorage *RemoteClientBridge::GetInput() {
    return &storage;
}

IMessageStorage *RemoteClientBridge::GetOutput() {
    return &storage;
}

BridgeInfo RemoteClientBridge::GetInfo() {
    return info;
}

void RemoteClientBridge::Commit() {
    // Get number of streams
    uint32_t streamCount;
    storage.ConsumeStreams(&streamCount, nullptr);

    // Get all streams
    streamCache.resize(streamCount);
    storage.ConsumeStreams(&streamCount, streamCache.data());

    // Push all streams
    for (const MessageStream &stream: streamCache) {
        MessageStreamHeaderProtocol protocol;
        protocol.schema = stream.GetSchema();
        protocol.versionID = stream.GetVersionID();
        protocol.size = stream.GetByteSize();

        // Send header and stream data (sync)
        client->WriteAsync(&protocol, sizeof(protocol));
        client->WriteAsync(stream.GetDataBegin(), protocol.size);

        // Tracking
        info.bytesWritten += sizeof(protocol);
        info.bytesWritten += protocol.size;
    }

    // Commit all inbound streams
    memoryBridge.Commit();
}
