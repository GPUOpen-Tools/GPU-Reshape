#include <catch2/catch.hpp>

// Backend
#include <Backend/IFeature.h>

// Message
#include <Message/OrderedMessageStorage.h>

// Messages
#include <Schemas/Feature.h>

class TestFeatureHook final : public IFeature {
public:
    FeatureHookTable GetHookTable() final {
        FeatureHookTable table{};
        table.drawIndexed = BindDelegate(this, TestFeatureHook::OnDrawIndexed);
        return table;
    }

    void CollectMessages(IMessageStorage *storage) override {

    }

    uint32_t testIndexCount = 0;

protected:
    void OnDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
        testIndexCount = indexCount;
    }
};

TEST_CASE("Backend.FeatureHook") {
    TestFeatureHook feature;

    FeatureHookTable table = feature.GetHookTable();

    table.drawIndexed.Invoke(5, 0, 0, 0, 0);
    REQUIRE(feature.testIndexCount == 5);
}

class TestFeatureMessage final : public IFeature {
public:
    FeatureHookTable GetHookTable() final {
        FeatureHookTable table{};
        table.drawIndexed = BindDelegate(this, TestFeatureMessage::OnDrawIndexed);
        return table;
    }

    void CollectMessages(IMessageStorage *storage) override {
        storage->AddStreamAndSwap(messages);
    }

    MessageStream messages;

protected:
    void OnDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
        if (!indexCount || !instanceCount) {
            MessageStreamView<EmptyDrawCommandMessage> view(messages);

            EmptyDrawCommandMessage *msg = view.Add();
            msg->indexCount = indexCount;
            msg->instanceCount = instanceCount;
        }
    }
};

TEST_CASE("Backend.FeatureMessage") {
    TestFeatureMessage feature;

    FeatureHookTable table = feature.GetHookTable();
    table.drawIndexed.Invoke(5, 1, 0, 0, 0);
    table.drawIndexed.Invoke(5, 0, 0, 0, 0);
    table.drawIndexed.Invoke(0, 1, 0, 0, 0);
    table.drawIndexed.Invoke(5, 1, 0, 0, 0);

    OrderedMessageStorage storage;
    feature.CollectMessages(&storage);

    uint32_t consumeCount;
    storage.ConsumeStreams(&consumeCount, nullptr);

    REQUIRE(consumeCount == 1);

    MessageStream stream;
    storage.ConsumeStreams(&consumeCount, &stream);

    REQUIRE(stream.GetCount() == 2);
    REQUIRE(stream.Is<EmptyDrawCommandMessage>());

    MessageStreamView<EmptyDrawCommandMessage> view(stream);

    auto it = view.GetIterator();

    // Validate messages
    REQUIRE(it->indexCount == 5);
    REQUIRE(it->instanceCount == 0);
    ++it;
    REQUIRE(it->indexCount == 0);
    REQUIRE(it->instanceCount == 1);
}
