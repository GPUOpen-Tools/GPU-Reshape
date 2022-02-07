// Discovery
#include <Services/Discovery/DiscoveryService.h>

// Schemas
#include <Schemas/Log.h>
#include <Schemas/PingPong.h>
#include <Schemas/Pipeline.h>

// Common
#include <Common/Console/ConsoleDevice.h>

// Bridge
#include <Bridge/NetworkBridge.h>
#include <Bridge/Log/LogConsoleListener.h>

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

int main(int32_t argc, const char* const* argv) {
    DiscoveryService service;

    // Info
    std::cout << "Standalone Discovery Service\n\n";

    // Attempt to install service
    std::cout << "Starting services... " << std::flush;
    {
        if (!service.Install()) {
            std::cerr << "Failed to install service\n";
            return 1;
        }

        // OK
        std::cout << "OK." << std::endl;
    }

    // Get registry
    Registry* registry = service.GetRegistry();

    // TODO: This is quite ugly... (needs releasing too)
    std::thread commitThread;

    // Network handle
    ComRef<NetworkBridge> network;

    // Attempt to start bridge
    std::cout << "Starting network bridge... " << std::flush;
    {
        network = registry->AddNew<NetworkBridge>();

        // Set up resolve
        EndpointResolve resolve;
        resolve.ipvxAddress = "127.0.0.1";

        // Attempt to install as client
        if (!network->InstallClient(resolve)) {
            std::cerr << "Failed to start network bridge\n";
            return 1;
        }

        // Install listeners
        network->Register(LogMessage::kID, registry->New<LogConsoleListener>());
        network->Register(PingPongMessage::kID, registry->New<PingPongConsole>());

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

        // Ping pong?
        else if (console.Is("global")) {
            MessageStreamView view(stream);

            // Enable global instrumentation
            auto* global = view.Add<SetGlobalInstrumentationMessage>();
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
