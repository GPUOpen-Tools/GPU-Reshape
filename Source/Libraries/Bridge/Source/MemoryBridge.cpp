// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

#include <Bridge/MemoryBridge.h>
#include <Bridge/IBridgeListener.h>

// Message
#include <Message/MessageStream.h>

void MemoryBridge::Register(MessageID mid, const ComRef<IBridgeListener>& listener) {
    MutexGuard guard(mutex);
    
    MessageBucket& bucket = buckets[mid];
    bucket.listeners.push_back(listener);
}

void MemoryBridge::Deregister(MessageID mid, const ComRef<IBridgeListener>& listener) {
    MutexGuard guard(mutex);
    MessageBucket& bucket = buckets[mid];

    auto&& it = std::find(bucket.listeners.begin(), bucket.listeners.end(), listener);
    if (it != bucket.listeners.end()) {
        bucket.listeners.erase(it);
    }
}

void MemoryBridge::Register(const ComRef<IBridgeListener>& listener) {
    MutexGuard guard(mutex);
    orderedListeners.push_back(listener);
}

void MemoryBridge::Deregister(const ComRef<IBridgeListener>& listener) {
    MutexGuard guard(mutex);
    
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

BridgeInfo MemoryBridge::GetInfo() {
    // Memory bridges do not keep track of written data
    return {};
}

void MemoryBridge::Commit() {
    MutexGuard guard(mutex);

    // Get the number of streams
    uint32_t streamCount;
    sharedStorage.ConsumeStreams(&streamCount, nullptr);

    // Consume all streams
    storageConsumeCache.clear();
    storageConsumeCache.resize(streamCount);
    sharedStorage.ConsumeStreams(&streamCount, storageConsumeCache.data());

    // Invoke streams in-order
    // TODO: This can be bucketed pretty easily if performance becomes a problem
    for (MessageStream& stream : storageConsumeCache) {
        if (stream.GetSchema().type == MessageSchemaType::Ordered) {
            for (const ComRef<IBridgeListener>& listener : orderedListeners) {
                listener->Handle(&stream, 1u);
            }
        } else {
            // No listener?
            auto bucketIt = buckets.find(stream.GetSchema().id);
            if (bucketIt == buckets.end()) {
                // TODO: Log warning
                continue;
            }

            // Get the bucket
            MessageBucket& bucket = bucketIt->second;

            // Pass through all listeners
            for (const ComRef<IBridgeListener>& listener : bucket.listeners) {
                listener->Handle(&stream, 1);
            }
        }
    }
}
