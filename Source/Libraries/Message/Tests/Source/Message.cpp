#include <catch2/catch.hpp>

#include <Message/IMessageStorage.h>
#include <Message/MessageStream.h>
#include <Message/MessageContainers.h>

// Schema
#include <Schemas/Schema.h>

/// Simple storage type
class MessageStorage final : public IMessageStorage {
public:
    void AddStream(const MessageStream &stream) override {
        storage.push_back(stream);
    }

    void AddStreamAndSwap(MessageStream& stream) override {
        storage.push_back(stream);
        stream.Clear();
    }

    void ConsumeStreams(uint32_t *count, MessageStream *streams) override {
        if (count) {
            *count = storage.size();
        }

        if (streams) {
            for (uint32_t i = 0; i < *count; i++) {
                streams[i].Swap(storage[i]);
            }

            storage.erase(storage.begin(), storage.begin() + *count);
        }
    }

    void Free(const MessageStream& stream) override {

    }

    std::vector<MessageStream> storage;
};

TEST_CASE("Message.StaticSchema") {
    MessageStream stream;

    // Static schema
    MessageStreamView<FooMessage> view(stream);

    // Add basic messages
    view.Add();
    view.Add();
    view.Add();

    // Pass through storage
    MessageStorage storage;
    storage.AddStreamAndSwap(stream);

    uint32_t consumeCount;
    storage.ConsumeStreams(&consumeCount, nullptr);

    std::vector<MessageStream> consumeStreams(consumeCount);
    storage.ConsumeStreams(&consumeCount, consumeStreams.data());

    REQUIRE(consumeStreams.size() == 1);

    for (auto it = MessageStreamView<FooMessage>(consumeStreams[0]).GetIterator(); it; ++it) {
        REQUIRE(it->life == 42);
    }
}

TEST_CASE("Message.DynamicSchema") {
    MessageStream stream;

    // Dynamic schema
    MessageStreamView<InstructionPixelInvocationDebugMessage> streamView(stream);

    // Add dynamic messages
    auto msgInA = streamView.Add(InstructionPixelInvocationDebugMessage::AllocationInfo { .dataCount = 512 });
    std::memset(msgInA->data.Get(), 0x0, sizeof(float) * 512);

    auto msgInB = streamView.Add(InstructionPixelInvocationDebugMessage::AllocationInfo { .dataCount = 512 });
    std::memset(msgInB->data.Get(), 0x0, sizeof(float) * 512);

    // Pass through storage
    MessageStorage storage;
    storage.AddStreamAndSwap(stream);

    uint32_t consumeCount;
    storage.ConsumeStreams(&consumeCount, nullptr);

    std::vector<MessageStream> consumeStreams(consumeCount);
    storage.ConsumeStreams(&consumeCount, consumeStreams.data());

    REQUIRE(consumeStreams.size() == 1);

    REQUIRE(consumeStreams[0].GetCount() == 2);

    for (auto it = MessageStreamView<InstructionPixelInvocationDebugMessage>(consumeStreams[0]).GetIterator(); it; ++it) {
        REQUIRE(it->data.count == 512);
    }
}

TEST_CASE("Message.OrderedSchema") {
    MessageStream stream;

    // Ordered schema
    MessageStreamView streamView(stream);

    // Static
    streamView.Add<FooMessage>();

    // Dynamic
    auto msgInA = streamView.Add<InstructionPixelInvocationDebugMessage>(InstructionPixelInvocationDebugMessage::AllocationInfo { .dataCount = 512 });
    std::memset(msgInA->data.Get(), 0x0, sizeof(float) * 512);

    auto msgInB = streamView.Add<InstructionPixelInvocationDebugMessage>(InstructionPixelInvocationDebugMessage::AllocationInfo { .dataCount = 512 });
    std::memset(msgInB->data.Get(), 0x0, sizeof(float) * 512);

    // Pass through storage
    MessageStorage storage;
    storage.AddStreamAndSwap(stream);

    uint32_t consumeCount;
    storage.ConsumeStreams(&consumeCount, nullptr);

    std::vector<MessageStream> consumeStreams(consumeCount);
    storage.ConsumeStreams(&consumeCount, consumeStreams.data());

    REQUIRE(consumeStreams.size() == 1);

    REQUIRE(consumeStreams[0].GetCount() == 3);

    // Ordered iteration
    for (auto it = MessageStreamView(consumeStreams[0]).GetIterator(); it; ++it) {
        switch (it.GetID()) {
            case FooMessage::kID: {
                REQUIRE(it.Get<FooMessage>()->life == 42);
                break;
            }
            case InstructionPixelInvocationDebugMessage::kID: {
                REQUIRE(it.Get<InstructionPixelInvocationDebugMessage>()->data.count == 512);
                break;
            }
        }
    }
}

/*TEST_CASE("Message.Bridge.Memory") {
    MessageRegistry registry;

    auto id = registry.Allocate();

    MessageStorage storage;

    MessageStream stream(id);
    stream.Insert(FooMessage());
    stream.Insert(FooMessage());
    stream.Insert(FooMessage());

    storage.AddStreamAndSwap(stream);

    MessageStorageSerializer storageSerializer;

    BridgeRegistry bridgeRegistry;

    BridgeID bridgeID = bridgeRegistry.Allocate(storageSerializer);

    bridge.AddSerializer(bridgeID, storage);

    bridge.Enqueue(bridgeID, storage);

    REQUIRE(consumeStreams.size() == 1);
}*/
