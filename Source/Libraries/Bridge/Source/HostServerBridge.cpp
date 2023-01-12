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

    // Local info
    AsioHostClientInfo asioInfo;
    strcpy_s(asioInfo.applicationName, config.applicationName);
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
    return &storage;
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
