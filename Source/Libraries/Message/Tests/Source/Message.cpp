// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#include <catch2/catch.hpp>

#include <Message/IMessageStorage.h>
#include <Message/MessageStream.h>
#include <Message/MessageContainers.h>
#include <Message/OrderedMessageStorage.h>

// Schema
#include <Schemas/Schema.h>

TEST_CASE("Message.StaticSchema") {
    MessageStream stream;

    // Static schema
    MessageStreamView<FooMessage> view(stream);

    // Add basic messages
    view.Add();
    view.Add();
    view.Add();

    // Pass through storage
    OrderedMessageStorage storage;
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
    OrderedMessageStorage storage;
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
    OrderedMessageStorage storage;
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
