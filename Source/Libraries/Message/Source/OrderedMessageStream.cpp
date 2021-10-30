#include <Message/OrderedMessageStorage.h>
#include <Message/MessageStream.h>

void OrderedMessageStorage::AddStream(const MessageStream &stream) {
    // Ignore if empty
    if (stream.IsEmpty()) {
        return;
    }

    // Add to storage, no recycling
    storage.push_back(stream);
}

void OrderedMessageStorage::AddStreamAndSwap(MessageStream &stream) {
    MessageSchema schema = stream.GetSchema();

    // Ignore if empty
    if (stream.IsEmpty()) {
        return;
    }

    // Target stream
    MessageStream target;

    // Ordered?
    if (schema == OrderedMessageSchema::GetSchema()) {
        // Pop one if possible
        if (!freeOrderedStreams.empty()) {
            target.Swap(freeOrderedStreams.back());
            freeOrderedStreams.pop_back();
        }
    } else {
        MessageBucket& bucket = messageBuckets[schema.id];

        // Pop one if possible
        if (!bucket.freeStreams.empty()) {
            target.Swap(bucket.freeStreams.back());
            bucket.freeStreams.pop_back();
        }
    }

    // Swap with target
    //  The target stream, possibly recycled, contains the produced messages,
    //  and the source stream swapped.
    target.Swap(stream);

    // Add the target
    storage.push_back(target);
}

void OrderedMessageStorage::ConsumeStreams(uint32_t *count, MessageStream *streams) {
    if (streams) {
        for (uint32_t i = 0; i < *count; i++) {
            streams[i].Swap(storage[i]);
        }

        storage.erase(storage.begin(), storage.begin() + *count);
    } else if (count) {
        *count = storage.size();
    }
}

void OrderedMessageStorage::Free(const MessageStream &stream) {
    MessageSchema schema = stream.GetSchema();

    // If the schema is not assigned, there is no purpose in recycling it
    if (schema.type == MessageSchemaType::None) {
        return;
    }

    // Ordered?
    if (schema == OrderedMessageSchema::GetSchema()) {
        freeOrderedStreams.push_back(stream);
        return;
    }

    // Let the bucket acquire it
    MessageBucket& bucket = messageBuckets[schema.id];
    bucket.freeStreams.push_back(stream);
}
