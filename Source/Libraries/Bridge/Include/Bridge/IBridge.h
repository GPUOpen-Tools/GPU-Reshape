// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
#include "BridgeInfo.h"

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Message
#include <Message/Message.h>

// Forward declarations
class IMessageStorage;
class IBridgeListener;

/// Bridge, responsible to transferring messages across components, potentially across network and process boundaries
class IBridge : public TComponent<IBridge> {
public:
    COMPONENT(IBridge);

    /// Register a listener
    /// \param mid the message id to listen for
    /// \param listener the listener
    virtual void Register(MessageID mid, const ComRef<IBridgeListener>& listener) = 0;

    /// Deregister a listener
    /// \param mid the listened message id
    /// \param listener the listener to be removed
    virtual void Deregister(MessageID mid, const ComRef<IBridgeListener>& listener) = 0;

    /// Register an unspecialized listener for ordered messages
    /// \param listener the listener
    virtual void Register(const ComRef<IBridgeListener>& listener) = 0;

    /// Deregister an unspecialized listener for ordered messages
    /// \param listener the listener to be removed
    virtual void Deregister(const ComRef<IBridgeListener>& listener) = 0;

    /// Get the bridge info
    virtual BridgeInfo GetInfo() = 0;

    /// Get the input storage
    virtual IMessageStorage* GetInput() = 0;

    /// Get the output storage
    virtual IMessageStorage* GetOutput() = 0;

    /// Commit all messages
    virtual void Commit() = 0;
};
