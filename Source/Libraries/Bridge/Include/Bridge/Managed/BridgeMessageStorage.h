#pragma once

#include "IMessageStorage.h"
#include "EndpointConfig.h"

class IMessageStorage;

namespace Bridge::CLR {
    public ref class BridgeMessageStorage : public IMessageStorage {
    public:
        BridgeMessageStorage(::IMessageStorage* storage);
        ~BridgeMessageStorage();

        // Overrides
        virtual void AddStream(Message::CLR::IMessageStream^ stream);

    private:
        /// Native data
        ::IMessageStorage* storage{ nullptr };
    };
}
