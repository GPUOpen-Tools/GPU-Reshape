#pragma once

#include "IBridge.h"
#include "EndpointConfig.h"

class Registry;
class RemoteClientBridge;

namespace Bridge::CLR {
    public ref class RemoteClientBridge : public IBridge {
    public:
        RemoteClientBridge();
        ~RemoteClientBridge();

        /// Install this bridge
        /// \param resolve configuration
        /// \return success state
        bool Install(const EndpointResolve^ resolve);

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
        /// Native data
        struct RemoteClientBridgePrivate* _private;
    };
}
