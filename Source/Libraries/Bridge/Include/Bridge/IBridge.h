#pragma once

// Common
#include <Common/IComponent.h>

// Message
#include <Message/Message.h>

// Forward declarations
class IMessageStorage;
class IBridgeListener;

/// Bridge, responsible to transferring messages across components, potentially across network and process boundaries
class IBridge : public IComponent {
public:
    COMPONENT(IBridge);

    /// Register a listener
    /// \param mid the message id to listen for
    /// \param listener the listener
    virtual void Register(MessageID mid, IBridgeListener* listener) = 0;

    /// Deregister a listener
    /// \param mid the listened message id
    /// \param listener the listener to be removed
    virtual void Deregister(MessageID mid, IBridgeListener* listener) = 0;

    /// Register an unspecialized listener for ordered messages
    /// \param listener the listener
    virtual void Register(IBridgeListener* listener) = 0;

    /// Deregister an unspecialized listener for ordered messages
    /// \param listener the listener to be removed
    virtual void Deregister(IBridgeListener* listener) = 0;

    /// Get the input storage
    virtual IMessageStorage* GetInput() = 0;

    /// Get the output storage
    virtual IMessageStorage* GetOutput() = 0;

    /// Commit all messages
    virtual void Commit() = 0;
};
