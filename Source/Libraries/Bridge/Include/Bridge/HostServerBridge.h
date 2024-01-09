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

// Bridge
#include "MemoryBridge.h"
#include "EndpointConfig.h"

// Std
#include <thread>
#include <atomic>
#include <condition_variable>

// Forward declarations
struct AsioHostServer;

/// Network Bridge
class HostServerBridge final : public IBridge {
public:
    ~HostServerBridge();

    /// Install this bridge
    /// \return success state
    bool Install(const EndpointConfig& config);

    /// Update the device configuration
    /// \param config given configuration
    void UpdateDeviceConfig(const EndpointDeviceConfig& config);

    /// Overrides
    void Register(MessageID mid, const ComRef<IBridgeListener>& listener) override;
    void Deregister(MessageID mid, const ComRef<IBridgeListener>& listener) override;
    void Register(const ComRef<IBridgeListener>& listener) override;
    void Deregister(const ComRef<IBridgeListener>& listener) override;
    IMessageStorage *GetInput() override;
    IMessageStorage *GetOutput() override;
    BridgeInfo GetInfo() override;
    void Commit() override;

private:
    /// Async read callback
    /// \param data the enqueued data
    /// \param size the enqueued size
    /// \return number of consumed bytes
    uint64_t OnReadAsync(const void* data, uint64_t size);

private:
    /// Current endpoint
    AsioHostServer* server{nullptr};

    /// Local storage
    OrderedMessageStorage storage;

    /// Piggybacked memory bridge
    MemoryBridge memoryBridge;

    /// Info across lifetime
    BridgeInfo info;

    /// Asio client information
    AsioHostClientInfo asioInfo{};

    /// Cache for commits
    std::vector<MessageStream> streamCache;
};
