#pragma once

#include <Bridge/IBridgeListener.h>

// Managed
#include <vcclr.h>

namespace Bridge::CLR {
    public class BridgeListenerInterop : public ::IBridgeListener {
    public:
        COMPONENT(BridgeListenerInterop);

        /// Constructor
        BridgeListenerInterop(const gcroot<Bridge::CLR::IBridgeListener^>& handle) : handle(handle) {

        }

        /// Overrides
        virtual void Handle(const ::MessageStream* streams, uint32_t count) override {
            // TODO: Batch submission
            for (uint32_t i = 0; i < count; i++) {
                handle->Handle(gcnew Message::CLR::MessageStream(), 1);
            }
        }

        /// Pinned handle
        gcroot<Bridge::CLR::IBridgeListener^> handle;
    };
}
