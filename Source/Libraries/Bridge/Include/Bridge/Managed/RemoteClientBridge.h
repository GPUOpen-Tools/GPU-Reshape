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
