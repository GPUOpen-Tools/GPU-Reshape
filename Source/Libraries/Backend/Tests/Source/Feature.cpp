#include <catch2/catch.hpp>

// Backend
#include <Backend/IFeature.h>

// Message
#include <Message/OrderedMessageStorage.h>

// Messages
#include <Schemas/Feature.h>
#include <Schemas/Instrumentation.h>
#include <Schemas/ShaderMetadata.h>

class TestFeatureHook final : public IFeature {
public:
    FeatureInfo GetInfo() override {
        return FeatureInfo();
    }

    FeatureHookTable GetHookTable() final {
        FeatureHookTable table{};
        table.drawIndexedInstanced = BindDelegate(this, TestFeatureHook::OnDrawIndexed);
        return table;
    }

    void CollectMessages(IMessageStorage *storage) override {

    }

    bool Install() override {
        return true;
    }

    uint32_t testIndexCount = 0;

protected:
    void OnDrawIndexed(CommandContext* context, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
        testIndexCount = indexCount;
    }
};

TEST_CASE("Backend.FeatureHook") {
    TestFeatureHook feature;

    FeatureHookTable table = feature.GetHookTable();

    table.drawIndexedInstanced.Invoke(nullptr, 5, 0, 0, 0, 0);
    REQUIRE(feature.testIndexCount == 5);
}

class TestFeatureMessage final : public IFeature {
public:
    FeatureInfo GetInfo() override {
        return FeatureInfo();
    }

    FeatureHookTable GetHookTable() final {
        FeatureHookTable table{};
        table.drawIndexedInstanced = BindDelegate(this, TestFeatureMessage::OnDrawIndexed);
        return table;
    }

    void CollectMessages(IMessageStorage *storage) override {
        storage->AddStreamAndSwap(messages);
    }

    bool Install() override {
        return true;
    }

    MessageStream messages;

protected:
    void OnDrawIndexed(CommandContext* context, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
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
    table.drawIndexedInstanced.Invoke(nullptr, 5, 1, 0, 0, 0);
    table.drawIndexedInstanced.Invoke(nullptr, 5, 0, 0, 0, 0);
    table.drawIndexedInstanced.Invoke(nullptr, 0, 1, 0, 0, 0);
    table.drawIndexedInstanced.Invoke(nullptr, 5, 1, 0, 0, 0);

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

class TestFeatureDynamicMixed final : public IFeature {
public:
    FeatureInfo GetInfo() override {
        return FeatureInfo();
    }

    FeatureHookTable GetHookTable() final {
        FeatureHookTable table{};
        table.drawIndexedInstanced = BindDelegate(this, TestFeatureDynamicMixed::OnDrawIndexed);
        return table;
    }

    void CollectMessages(IMessageStorage *storage) override {
        storage->AddStreamAndSwap(emptyDrawMessages);
        storage->AddStreamAndSwap(complexMessages);
    }

    bool Install() override {
        return true;
    }

    MessageStream emptyDrawMessages;
    MessageStream complexMessages;

protected:
    void OnDrawIndexed(CommandContext* context, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
        if (!indexCount || !instanceCount) {
            MessageStreamView<EmptyDrawCommandMessage> view(emptyDrawMessages);

            EmptyDrawCommandMessage *msg = view.Add();
            msg->indexCount = indexCount;
            msg->instanceCount = instanceCount;
        }

        MessageStreamView<ComplexMessageMessage> complexView(complexMessages);

        ComplexMessageMessage *complexMsg = complexView.Add(ComplexMessageMessage::AllocationInfo { .dataCount = 8 });
        for (uint64_t i = 0; i < complexMsg->data.count; i++) {
            complexMsg->data[i] = i;
        }
    }
};

TEST_CASE("Backend.FeatureMessageDynamicMixed") {
    TestFeatureDynamicMixed feature;

    FeatureHookTable table = feature.GetHookTable();
    table.drawIndexedInstanced.Invoke(nullptr, 5, 1, 0, 0, 0);
    table.drawIndexedInstanced.Invoke(nullptr, 5, 0, 0, 0, 0);
    table.drawIndexedInstanced.Invoke(nullptr, 0, 1, 0, 0, 0);
    table.drawIndexedInstanced.Invoke(nullptr, 5, 1, 0, 0, 0);

    OrderedMessageStorage storage;
    feature.CollectMessages(&storage);

    uint32_t consumeCount;
    storage.ConsumeStreams(&consumeCount, nullptr);

    REQUIRE(consumeCount == 2);

    std::vector<MessageStream> consumedStreams(consumeCount);
    storage.ConsumeStreams(&consumeCount, consumedStreams.data());

    REQUIRE(consumedStreams[0].Is<EmptyDrawCommandMessage>());
    REQUIRE(consumedStreams[1].Is<ComplexMessageMessage>());

    MessageStreamView<EmptyDrawCommandMessage> view(consumedStreams[0]);
    REQUIRE(view.GetStream().GetCount() == 2);

    MessageStreamView<ComplexMessageMessage> complexView(consumedStreams[1]);
    REQUIRE(complexView.GetStream().GetCount() == 4);

    {
        auto it = view.GetIterator();

        REQUIRE(it->indexCount == 5);
        REQUIRE(it->instanceCount == 0);

        ++it;

        REQUIRE(it->indexCount == 0);
        REQUIRE(it->instanceCount == 1);
    }

    {
        auto it = complexView.GetIterator();

        REQUIRE(it->data.count == 8);
        for (uint64_t i = 0; i < it->data.count; i++) {
            REQUIRE(it->data[i] == i);
        }

        ++it;

        REQUIRE(it->data.count == 8);
        for (uint64_t i = 0; i < it->data.count; i++) {
            REQUIRE(it->data[i] == i);
        }
    }
}
