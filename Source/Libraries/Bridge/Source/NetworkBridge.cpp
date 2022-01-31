#include <Bridge/NetworkBridge.h>
#include <Bridge/IBridgeListener.h>
#include <Bridge/EndpointConfig.h>
#include <Bridge/NetworkProtocol.h>
#include <Bridge/Asio/AsioClient.h>
#include <Bridge/Asio/AsioServer.h>

// Message
#include <Message/MessageStream.h>

bool NetworkBridge::InstallServer(const EndpointConfig &config) {
    // Create endpoint
    endpoint = new(allocators) AsioServer(config.port);

    // Set read
    endpoint->SetReadCallback([this](const void *data, uint64_t size) {
        return OnReadAsync(data, size);
    });

    // Start worker
    workerThread = std::thread([this] { Worker(); });

    // OK
    return true;
}

bool NetworkBridge::InstallClient(const EndpointResolve &resolve) {
    // Create endpoint
    endpoint = new(allocators) AsioClient(resolve.ipvxAddress, resolve.config.port);

    // Set read
    endpoint->SetReadCallback([this](const void *data, uint64_t size) {
        return OnReadAsync(data, size);
    });

    // Start worker
    workerThread = std::thread([this] { Worker(); });

    // OK
    return true;
}

NetworkBridge::~NetworkBridge() {
    // Wait for worker
    workerThread.join();

    // Release endpoint
    destroy(endpoint, allocators);
}

void NetworkBridge::Worker() {
    endpoint->Run();
}

uint64_t NetworkBridge::OnReadAsync(const void *data, uint64_t size) {
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

    // Consume entire stream
    return sizeof(MessageStreamHeaderProtocol) + protocol->size;
}

void NetworkBridge::Register(MessageID mid, IBridgeListener *listener) {
    memoryBridge.Register(mid, listener);
}

void NetworkBridge::Deregister(MessageID mid, IBridgeListener *listener) {
    memoryBridge.Deregister(mid, listener);
}

void NetworkBridge::Register(IBridgeListener *listener) {
    memoryBridge.Register(listener);
}

void NetworkBridge::Deregister(IBridgeListener *listener) {
    memoryBridge.Deregister(listener);
}

IMessageStorage *NetworkBridge::GetInput() {
    return &storage;
}

IMessageStorage *NetworkBridge::GetOutput() {
    return &storage;
}

void NetworkBridge::Commit() {
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
        endpoint->WriteAsync(&protocol, sizeof(protocol));
        endpoint->WriteAsync(stream.GetDataBegin(), protocol.size);
    }

    // Commit all inbound streams
    memoryBridge.Commit();
}
