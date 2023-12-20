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

#pragma once

// Message
#include "IMessageStorage.h"
#include "MessageStream.h"

// Common
#include <Common/Dispatcher/Mutex.h>

// Std
#include <vector>
#include <mutex>
#include <map>

/// Simple batch ordered message storage
class OrderedMessageStorage final : public IMessageStorage {
public:
    /// Overrides
    void AddStream(const MessageStream &stream) override;
    void AddStreamAndSwap(MessageStream& stream) override;
    void ConsumeStreams(uint32_t *count, MessageStream *streams) override;
    void Free(const MessageStream& stream) override;
    uint32_t StreamCount() override;

private:
    Mutex mutex;

    /// Cache for message types
    struct MessageBucket {
        std::vector<MessageStream> freeStreams;
    };

    /// All message buckets
    std::map<MessageID, MessageBucket> messageBuckets;

    /// Free ordered streams, message invariant
    std::vector<MessageStream> freeOrderedStreams;

    /// Current pushed streams
    std::vector<MessageStream> storage;
};
