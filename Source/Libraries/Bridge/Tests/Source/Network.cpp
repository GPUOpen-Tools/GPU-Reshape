#include <catch2/catch.hpp>

// Bridge
#include <Bridge/NetworkBridge.h>
#include <Bridge/IBridgeListener.h>

// Schemas
#include <Schemas/Log.h>

// Common
#include <Common/Registry.h>

class Listener : public TComponent<Listener>, public IBridgeListener {
public:
    COMPONENT(Listener);

    void Handle(const MessageStream *streams, uint32_t count) override {
        for (uint32_t i = 0; i < count; i++) {
            ConstMessageStreamView<LogMessage> view(streams[i]);

            for (auto it = view.GetIterator(); it; ++it) {
                REQUIRE(it->message.View() == "Hello World");
            }
        }

        visited = true;
    }

    bool visited{false};
};

TEST_CASE("Bridge.Network") {
    Registry registry;

    auto* server = registry.New<NetworkBridge>();
    REQUIRE(server->InstallServer(EndpointConfig{}));

    auto* client = registry.New<NetworkBridge>();
    REQUIRE(client->InstallClient(EndpointResolve {
        .ipvxAddress = "127.0.0.1"
    }));

    // Wait for the operations to finish (very dirty)
    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto* serverListener = registry.New<Listener>();
    server->Register(LogMessage::kID, serverListener);

    auto* clientListener = registry.New<Listener>();
    client->Register(LogMessage::kID, clientListener);

    MessageStream stream;
    MessageStreamView<LogMessage> view(stream);

    LogMessage* message = view.Add(LogMessage::AllocationInfo{
        .systemLength = 0,
        .messageLength = sizeof("Hello World") - 1
    });

    message->message.Set("Hello World");

    server->GetOutput()->AddStream(stream);
    client->GetOutput()->AddStream(stream);

    client->Commit();
    server->Commit();

    // Wait for the operations to finish (very dirty)
    std::this_thread::sleep_for(std::chrono::seconds(1));

    client->Commit();
    server->Commit();

    REQUIRE(serverListener->visited);
    REQUIRE(clientListener->visited);
}
