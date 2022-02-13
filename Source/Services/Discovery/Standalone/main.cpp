// Discovery
#include <Services/Discovery/DiscoveryService.h>

// Schemas
#include <Schemas/Log.h>
#include <Schemas/PingPong.h>
#include <Schemas/HostResolve.h>
#include <Schemas/Pipeline.h>

// Common
#include <Common/Console/ConsoleDevice.h>

// Bridge
#include <Bridge/RemoteClientBridge.h>
#include <Bridge/Log/LogConsoleListener.h>

// Services
#include <Services/HostResolver/HostResolverService.h>

/// Console ping pong
class PingPongConsole : public TComponent<PingPongConsole>, public IBridgeListener {
public:
    COMPONENT(PingPongConsole);

    /// Overrides
    void Handle(const MessageStream *streams, uint32_t count) override {
        for (uint32_t i = 0; i < count; i++) {
            ConstMessageStreamView<PingPongMessage> view(streams[i]);

            for (auto it = view.GetIterator(); it; ++it) {
                std::cout << "pong\n" << std::flush;
            }
        }
    }
};

/// Generic ordered listener
class OrderedListener : public TComponent<OrderedListener>, public IBridgeListener {
public:
    COMPONENT(OrderedListener);

    /// Overrides
    void Handle(const MessageStream *streams, uint32_t count) override {
        for (uint32_t i = 0; i < count; i++) {
            ConstMessageStreamView view(streams[i]);

            for (auto it = view.GetIterator(); it; ++it) {
                switch (it.GetID()) {
                    case HostConnectedMessage::kID: {
                        OnClientConnected(it.Get<HostConnectedMessage>());
                        break;
                    }
                    case HostDiscoveryMessage::kID: {
                        OnDiscovery(it.Get<HostDiscoveryMessage>());
                        break;
                    }
                }
            }
        }
    }

private:
    void OnClientConnected(const HostConnectedMessage *discovery) {
        std::cout << "Client connected\n" << std::flush;
    }

    void OnDiscovery(const HostDiscoveryMessage *discovery) {
        std::cout << "Discovery:\n";

        ConstMessageStreamView view(discovery->infos);

        for (auto it = view.GetIterator(); it; ++it) {
            switch (it.GetID()) {
                case HostServerInfoMessage::kID: {
                    auto *entry = it.Get<HostServerInfoMessage>();

                    std::cout
                        << entry->process.View()
                        << " '" << entry->application.View() << "'"
                        << " " << entry->guid.View()
                        << "\n";
                }
            }
        }

        std::cout << std::flush;
    }
};

int main(int32_t argc, const char *const *argv) {
    DiscoveryService service;
    HostResolverService hostResolverService;

    // Info
    std::cout << "Standalone Discovery Service\n\n";

    // Attempt to install service
    std::cout << "Starting services... " << std::flush;
    {
        if (!service.Install()) {
            std::cerr << "Failed to install the discovery service\n";
            return 1;
        }

        // Install the host resolver
        //  ? Ensures that the host resolver is running on the system
        if (!hostResolverService.Install()) {
            std::cerr << "Failed to install the host resolver service\n";
            return 1;
        }

        // OK
        std::cout << "OK." << std::endl;
    }

    // Get registry
    Registry *registry = service.GetRegistry();

    // TODO: This is quite ugly... (needs releasing too)
    std::thread commitThread;

    // Network handle
    ComRef<RemoteClientBridge> network;

    // Attempt to start bridge
    std::cout << "Starting network bridge... " << std::flush;
    {
        // Create network
        network = registry->AddNew<RemoteClientBridge>();

        // Set up resolve
        EndpointResolve resolve;
        resolve.ipvxAddress = "127.0.0.1";

        // Attempt to install
        if (!network->Install(resolve)) {
            std::cerr << "Failed to start network bridge\n";
            return 1;
        }

        // Install listeners
        network->Register(LogMessage::kID, registry->New<LogConsoleListener>());
        network->Register(PingPongMessage::kID, registry->New<PingPongConsole>());
        network->Register(registry->New<OrderedListener>());

        // Commit helper
        commitThread = std::thread([network] {
            for (;;) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                network->Commit();
            }
        });

        // OK
        std::cout << "OK." << std::endl;
    }

    // Start device
    ConsoleDevice console;
    while (console.Next()) {
        MessageStream stream;

        // Ping pong?
        if (console.Is("ping")) {
            MessageStreamView<PingPongMessage> view(stream);
            view.Add();
        }

            // Start discovery?
        else if (console.Is("discovery")) {
            network->DiscoverAsync();
        }

            // Start discovery?
        else if (console.Is("client")) {
            auto guid = GlobalUID::FromString(console.Arg(0));
            if (!guid.IsValid()) {
                std::cerr << "Invalid GUID" << std::endl;
                continue;
            }

            network->RequestClientAsync(guid);
        }

            // Global instrumentation?
        else if (console.Is("global")) {
            MessageStreamView view(stream);

            // Enable global instrumentation
            auto *global = view.Add<SetGlobalInstrumentationMessage>();
            global->featureBitSet = ~0ul;
        }

            // Not known
        else {
            std::cout << "Unknown command '" << console.Command() << "'" << std::endl;
        }

        // Add stream to output
        network->GetOutput()->AddStream(stream);
    }

    // OK
    return 0;
}
