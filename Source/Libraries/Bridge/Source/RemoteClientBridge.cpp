#include <Bridge/RemoteClientBridge.h>
#include <Bridge/IBridgeListener.h>
#include <Bridge/EndpointConfig.h>
#include <Bridge/NetworkProtocol.h>
#include <Bridge/Asio/AsioRemoteClient.h>

// Message
#include <Message/MessageStream.h>

// Schemas
#include <Schemas/HostResolve.h>

bool RemoteClientBridge::Install(const EndpointResolve &resolve) {
    // Port config
    AsioRemoteConfig asioConfig;
    asioConfig.hostResolvePort = resolve.config.sharedPort;
    asioConfig.ipvxAddress = resolve.ipvxAddress;

    // Create the client
    client = new(allocators) AsioRemoteClient(asioConfig);

    // Subscribe
    client->onConnected.Add(0, [this] { OnConnected(); });
    client->onDiscovery.Add(0, [this](const AsioRemoteServerResolverDiscoveryRequest::Response &response) { OnDiscovery(response); });

    // Set read callback
    client->SetServerReadCallback([this](AsioSocketHandler &handler, const void *data, uint64_t size) {
        return OnReadAsync(data, size);
    });

    // OK
    return true;
}

void RemoteClientBridge::DiscoverAsync() {
    client->DiscoverAsync();
}

void RemoteClientBridge::RequestClientAsync(const AsioHostClientToken &guid) {
    client->RequestClientAsync(AsioHostClientToken(guid));
}

void RemoteClientBridge::OnConnected() {
    MessageStream stream;

    MessageStreamView view(stream);
    view.Add<HostConnectedMessage>();

    memoryBridge.GetOutput()->AddStream(stream);
}

void RemoteClientBridge::OnDiscovery(const AsioRemoteServerResolverDiscoveryRequest::Response &response) {
    // Entry stream
    MessageStream entries;
    {
        MessageStreamView view(entries);

        // Process all entries
        for (uint32_t i = 0; i < response.entryCount; i++) {
            const AsioRemoteServerResolverDiscoveryRequest::Entry &entry = response.entries[i];

            // Convert guid to string
            std::string guid = entry.token.ToString();

            // Allocate info
            auto *info = view.Add<HostServerInfoMessage>(HostServerInfoMessage::AllocationInfo{
                .guidLength = guid.length(),
                .processLength = std::strlen(entry.info.processName),
                .applicationLength = std::strlen(entry.info.applicationName)
            });

            // Set data
            info->guid.Set(guid);
            info->process.Set(entry.info.processName);
            info->application.Set(entry.info.applicationName);
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

RemoteClientBridge::~RemoteClientBridge() {
    // Release endpoint
    destroy(client, allocators);
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
    stream.SetData(static_cast<const uint8_t *>(data) + sizeof(MessageStreamHeaderProtocol), protocol->size, 0);
    memoryBridge.GetOutput()->AddStream(stream);

    // Commit all inbound streams if requested
    if (commitOnAppend) {
        memoryBridge.Commit();
    }

    // Consume entire stream
    return sizeof(MessageStreamHeaderProtocol) + protocol->size;
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
        protocol.size = stream.GetByteSize();

        // Send header and stream data (sync)
        client->WriteAsync(&protocol, sizeof(protocol));
        client->WriteAsync(stream.GetDataBegin(), protocol.size);
    }

    // Commit all inbound streams
    memoryBridge.Commit();
}
