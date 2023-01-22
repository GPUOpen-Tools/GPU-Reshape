// Discovery
#include <Services/Discovery/DiscoveryService.h>

// Schemas
#include <Schemas/Log.h>
#include <Schemas/PingPong.h>
#include <Schemas/HostResolve.h>
#include <Schemas/Instrumentation.h>
#include <Schemas/SGUID.h>

// Common
#include <Common/Console/ConsoleDevice.h>

// Bridge
#include <Bridge/RemoteClientBridge.h>
#include <Bridge/Log/LogConsoleListener.h>

// Backend
#include <Backend/ShaderSGUIDHostListener.h>

// Message
#include <Message/IMessageHub.h>

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

/// Generic message hub
class MessageHub : public IMessageHub {
public:
    /// Overrides
    void Add(const std::string_view &name, const std::string_view &message, uint32_t count) override {
        // Open console on demand
        if (!console) {
            console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
            SetConsoleActiveScreenBuffer(console);
        }

        // Attempt to find existing key
        auto &position = lookup[std::make_pair(std::string(name), std::string(message))];

        // If not found, allocate position
        if (position.cursor.X == 0xFFF) {
            // Get the current offset
            CONSOLE_SCREEN_BUFFER_INFO screenInfo;
            GetConsoleScreenBufferInfo(console, &screenInfo);

            // Compose the message
            std::stringstream ss;
            ss << name << " : " << message;

            // Write the message
            DWORD dwBytesWritten{0};
            WriteConsole(console, ss.str().c_str(), static_cast<uint32_t>(ss.str().length()), &dwBytesWritten, 0);

            // Store final position
            position.cursor = screenInfo.dwCursorPosition;
        }

        // Increment number of messages
        position.count += count;

        // Compose number, split thousands by '
        std::string num = std::to_string(position.count);
        for (int32_t i = static_cast<int32_t>(num.length()) - 3; i >= 1; i -= 3) {
            num.insert(num.begin() + i, '\'');
        }

        // Arbitrary printing location
        COORD pos = position.cursor;
        pos.X = 64;

        // Compose count
        std::stringstream ss;
        ss << "#" << num;

        // Write the count
        DWORD dwBytesWritten = 0;
        WriteConsoleOutputCharacter(console, ss.str().c_str(), static_cast<uint32_t>(ss.str().length()), pos, &dwBytesWritten);
    }

    ~MessageHub() {
        CloseHandle(console);
    }

private:
    struct Position {
        /// Cursor position
        COORD cursor{0xFFF, 0xFFF};

        /// Number of messages
        uint32_t count{0};
    };

    /// Terribly naive position cache
    std::map<std::pair<std::string, std::string>, Position> lookup;

    /// Win32 console handle
    HANDLE console{nullptr};
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

    // General resolver
    auto resolver = registry->New<PluginResolver>();

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

    // Attempt to start features
    std::cout << "Loading plugins ... " << std::flush;
    {
        // Install hub
        registry->AddNew<MessageHub>();

        // Install SGUID listener
        network->Register(ShaderSourceMappingMessage::kID, registry->AddNew<ShaderSGUIDHostListener>());

        // Find all frontend plugins
        PluginList list;
        if (!resolver->FindPlugins("frontend", &list)) {
            std::cerr << "Failed to find frontend plugins\n";
            return 1;
        }

        // Install plugins
        if (!resolver->InstallPlugins(list)) {
            std::cerr << "Failed to install frontend plugins\n";
            return 1;
        }

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
