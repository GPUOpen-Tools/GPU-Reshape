// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
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

#include <Message/OrderedMessageStorage.h>
#include <Message/MessageStream.h>

void OrderedMessageStorage::AddStream(const MessageStream &stream) {
    // Ignore if empty
    if (stream.IsEmpty()) {
        return;
    }

    // Add to storage, no recycling
    MutexGuard guard(mutex);
    storage.push_back(stream);
}

void OrderedMessageStorage::AddStreamAndSwap(MessageStream &stream) {
    MessageSchema schema = stream.GetSchema();

    // Ignore if empty
    if (stream.IsEmpty()) {
        return;
    }

    MutexGuard guard(mutex);

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
    MutexGuard guard(mutex);

    if (streams) {
        *count = std::min(*count, static_cast<uint32_t>(storage.size()));
        
        for (uint32_t i = 0; i < *count; i++) {
            streams[i].ClearWithSchemaInvalidate();
            streams[i].Swap(storage[i]);
        }

        storage.erase(storage.begin(), storage.begin() + *count);
    } else if (count) {
        *count = static_cast<uint32_t>(storage.size());
    }
}

void OrderedMessageStorage::Free(const MessageStream &stream) {
    MessageSchema schema = stream.GetSchema();

    // If the schema is not assigned, there is no purpose in recycling it
    if (schema.type == MessageSchemaType::None) {
        return;
    }

    MutexGuard guard(mutex);

    // Ordered?
    if (schema == OrderedMessageSchema::GetSchema()) {
        freeOrderedStreams.push_back(stream);
        return;
    }

    // Let the bucket acquire it
    MessageBucket& bucket = messageBuckets[schema.id];
    bucket.freeStreams.push_back(stream);
}

uint32_t OrderedMessageStorage::StreamCount() {
    MutexGuard guard(mutex);
    return static_cast<uint32_t>(storage.size());
}
