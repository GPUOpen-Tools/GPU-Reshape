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
