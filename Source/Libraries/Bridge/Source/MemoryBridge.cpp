#include <Bridge/MemoryBridge.h>
#include <Bridge/IBridgeListener.h>

// Message
#include <Message/MessageStream.h>

void MemoryBridge::Register(MessageID mid, IBridgeListener *listener) {
    MessageBucket& bucket = buckets[mid];
    bucket.listeners.push_back(listener);
}

void MemoryBridge::Deregister(MessageID mid, IBridgeListener *listener) {
    MessageBucket& bucket = buckets[mid];

    auto&& it = std::find(bucket.listeners.begin(), bucket.listeners.end(), listener);
    if (it != bucket.listeners.end()) {
        bucket.listeners.erase(it);
    }
}

void MemoryBridge::Register(IBridgeListener *listener) {
    orderedListeners.push_back(listener);
}

void MemoryBridge::Deregister(IBridgeListener *listener) {
    auto&& it = std::find(orderedListeners.begin(), orderedListeners.end(), listener);
    if (it != orderedListeners.end()) {
        orderedListeners.erase(it);
    }
}

IMessageStorage *MemoryBridge::GetInput() {
    return &sharedStorage;
}

IMessageStorage *MemoryBridge::GetOutput() {
    return &sharedStorage;
}

void MemoryBridge::Commit() {
    // Get the number of streams
    uint32_t streamCount;
    sharedStorage.ConsumeStreams(&streamCount, nullptr);

    // Consume all streams
    storageConsumeCache.clear();
    storageConsumeCache.resize(streamCount);
    sharedStorage.ConsumeStreams(&streamCount, storageConsumeCache.data());

    // TODO: Don't waste memory
    storageOrderedCache.clear();

    // Collect all ordered streams
    for (MessageStream& stream : storageConsumeCache) {
        // Skip unordered
        if (stream.GetSchema().type != MessageSchemaType::Ordered) {
            continue;
        }

        storageOrderedCache.emplace_back().Swap(stream);
    }

    // Invoke ordered listeners
    if (!storageOrderedCache.empty()) {
        for (IBridgeListener* listener : orderedListeners) {
            listener->Handle(storageOrderedCache.data(), static_cast<uint32_t>(storageOrderedCache.size()));
        }
    }

    // Process all streams
    for (const MessageStream& stream : storageConsumeCache) {
        MessageSchema schema = stream.GetSchema();

        // Skip ordered
        if (schema.type == MessageSchemaType::Ordered) {
            continue;
        }

        // No listener?
        auto bucketIt = buckets.find(schema.id);
        if (bucketIt == buckets.end()) {
            // TODO: Log warning
            continue;
        }

        // Get the bucket
        MessageBucket& bucket = bucketIt->second;

        // Pass through all listeners
        for (IBridgeListener* listener : bucket.listeners) {
            listener->Handle(&stream, 1);
        }
    }
}
