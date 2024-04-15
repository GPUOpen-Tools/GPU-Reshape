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

#pragma once

// Message
#include "Message.h"

struct MessageStream;

/// Message stream storage
class IMessageStorage {
public:
    /// Add a stream
    /// \param stream
    virtual void AddStream(const MessageStream& stream) = 0;

    /// Add and swap a stream
    /// ? Inbound stream is consumed, and recycled with an older container
    /// \param stream
    virtual void AddStreamAndSwap(MessageStream& stream) = 0;

    /// Consume added streams
    /// \param count always filled with the number of consumable / consumed streams
    /// \param streams if not null, filled with [count] streams
    virtual void ConsumeStreams(uint32_t* count, MessageStream* streams) = 0;

    /// Free a consumed message stream
    /// \param stream
    virtual void Free(const MessageStream& stream) = 0;

    /// Get the number of streams
    virtual uint32_t StreamCount() = 0;
};
