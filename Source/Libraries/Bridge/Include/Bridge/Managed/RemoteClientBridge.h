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

#include "IBridge.h"
#include "EndpointConfig.h"

// Forward declarations
class Registry;
class RemoteClientBridge;

namespace Bridge::CLR {
    // Forward declarations
    class BridgeListenerInterop;

    /// Asynchronous connection accepted handler
    public delegate void RemoteClientBridgeAsyncConnectedDelegate();

    public ref class RemoteClientBridge : public IBridge {
    public:
        RemoteClientBridge();
        ~RemoteClientBridge();

        /// Install this bridge
        /// \param resolve configuration
        /// \return success state
        bool Install(const EndpointResolve^ resolve);

        /// Install this bridge
        /// \param resolve configuration
        /// \return success state
        void InstallAsync(const EndpointResolve^ resolve);

        /// Set the asynchronous connection delegate
        /// \param delegate delegate
        void SetAsyncConnectedDelegate(RemoteClientBridgeAsyncConnectedDelegate^ delegate);

        /// Cancel pending requests
        void Cancel();

        /// Close existing connection
        void Stop();
        
        /// Submit a discovery request
        void DiscoverAsync();

        /// Send an async client request
        /// \param guid client guid, must originate from the discovery request
        void RequestClientAsync(System::Guid^ guid);

        /// Overrides
        virtual void Register(MessageID mid, IBridgeListener^ listener) sealed;
        virtual void Deregister(MessageID mid, IBridgeListener^ listener) sealed;
        virtual void Register(IBridgeListener^ listener) sealed;
        virtual void Deregister(IBridgeListener^ listener) sealed;
        virtual IMessageStorage^ GetInput() sealed;
        virtual IMessageStorage^ GetOutput() sealed;
        virtual BridgeInfo^ GetInfo() sealed;
        virtual void Commit() sealed;

        /// Enables auto commits on remote appends
        void SetCommitOnAppend(bool enabled);

    private:
        /// Intermediate entry
        ref struct InteropEntry {
            BridgeListenerInterop* component;
        };

        /// Message id to entry key
        using MIDKey = Collections::Generic::KeyValuePair<uint32_t, IBridgeListener^>;

        /// Ordered interop lookups
        Collections::Generic::Dictionary<IBridgeListener^, InteropEntry^> remoteInteropLookup;

        /// Static / dynamic interop lookups
        Collections::Generic::Dictionary<MIDKey, InteropEntry^> remoteKeyedInteropLookup;
        
    private:
        /// Native data
        struct RemoteClientBridgePrivate* _private;
    };
}
