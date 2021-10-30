#include <Message/OrderedMessageStorage.h>
#include <Message/MessageStream.h>

void OrderedMessageStorage::AddStream(const MessageStream &stream) {
    storage.push_back(stream);
}

void OrderedMessageStorage::AddStreamAndSwap(MessageStream &stream) {
    storage.push_back(stream);
    stream.Clear();
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
}
