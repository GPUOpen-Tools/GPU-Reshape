#pragma once

// Bridge
#include <Bridge/IBridge.h>
#include "IBridgeListener.h"
#include "IMessageStorage.h"
#include "BridgeInfo.h"

namespace Bridge::CLR {
    public interface class IBridge {
        /// Register a listener
        /// \param mid the message id to listen for
        /// \param listener the listener
        virtual void Register(MessageID mid, IBridgeListener^listener) = 0;

        /// Deregister a listener
        /// \param mid the listened message id
        /// \param listener the listener to be removed
        virtual void Deregister(MessageID mid, IBridgeListener^listener) = 0;

        /// Register an unspecialized listener for ordered messages
        /// \param listener the listener
        virtual void Register(IBridgeListener^listener) = 0;

        /// Deregister an unspecialized listener for ordered messages
        /// \param listener the listener to be removed
        virtual void Deregister(IBridgeListener^listener) = 0;

        /// Get the bridge diagnostic info
        virtual BridgeInfo^ GetInfo() = 0;

        /// Get the input storage
        virtual IMessageStorage ^ GetInput() = 0;

        /// Get the output storage
        virtual IMessageStorage ^ GetOutput() = 0;

        /// Commit all messages
        virtual void Commit() = 0;
    };
}
