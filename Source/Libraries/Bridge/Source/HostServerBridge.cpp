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

#include <Bridge/HostServerBridge.h>
#include <Bridge/IBridgeListener.h>
#include <Bridge/EndpointConfig.h>
#include <Bridge/NetworkProtocol.h>
#include <Bridge/Asio/AsioHostServer.h>

// Common
#include <Common/FileSystem.h>

// Message
#include <Message/MessageStream.h>

bool HostServerBridge::Install(const EndpointConfig &config) {
    // Port config
    AsioConfig asioConfig;
    asioConfig.hostResolvePort = config.sharedPort;
    asioConfig.reservedToken = config.reservedToken;

    // Local info
    AsioHostClientInfo asioInfo;
    strcpy_s(asioInfo.applicationName, config.applicationName);
    strcpy_s(asioInfo.apiName, config.apiName);
    strcpy_s(asioInfo.processName, GetCurrentExecutableName().c_str());

    // Platform info
#ifdef _WIN64
    asioInfo.processId = GetProcessId(GetCurrentProcess());
#endif

    // Create the server
    server = new(allocators) AsioHostServer(asioConfig, asioInfo);

    // Set read callback
    server->SetServerReadCallback([this](AsioSocketHandler& handler, const void *data, uint64_t size) {
        return OnReadAsync(data, size);
    });

    // OK
    return true;
}

HostServerBridge::~HostServerBridge() {
    // Release endpoint
    destroy(server, allocators);
}

uint64_t HostServerBridge::OnReadAsync(const void *data, uint64_t size) {
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
    stream.SetData(static_cast<const uint8_t*>(data) + sizeof(MessageStreamHeaderProtocol), protocol->size, 0);
    memoryBridge.GetOutput()->AddStream(stream);

    // Determine byte count
    const size_t bytes = sizeof(MessageStreamHeaderProtocol) + protocol->size;
    info.bytesRead += bytes;

    // Consume entire stream
    return bytes;
}

void HostServerBridge::Register(MessageID mid, const ComRef<IBridgeListener>& listener) {
    memoryBridge.Register(mid, listener);
}

void HostServerBridge::Deregister(MessageID mid, const ComRef<IBridgeListener>& listener) {
    memoryBridge.Deregister(mid, listener);
}

void HostServerBridge::Register(const ComRef<IBridgeListener>& listener) {
    memoryBridge.Register(listener);
}

void HostServerBridge::Deregister(const ComRef<IBridgeListener>& listener) {
    memoryBridge.Deregister(listener);
}

IMessageStorage *HostServerBridge::GetInput() {
    return memoryBridge.GetInput();
}

IMessageStorage *HostServerBridge::GetOutput() {
    return &storage;
}

BridgeInfo HostServerBridge::GetInfo() {
    return info;
}

void HostServerBridge::Commit() {
    // Endpoint must be in a good state
    if (!server->IsOpen()) {
        return;
    }

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
        server->BroadcastServerAsync(&protocol, sizeof(protocol));
        server->BroadcastServerAsync(stream.GetDataBegin(), protocol.size);

        // Tracking
        info.bytesWritten += sizeof(protocol);
        info.bytesWritten += protocol.size;
    }

    // Commit all inbound streams
    memoryBridge.Commit();
}
