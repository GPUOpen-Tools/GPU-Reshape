#include <Bridge/Network/PingPongListener.h>
#include <Bridge/IBridge.h>

// Message
#include <Message/MessageStream.h>
#include <Message/IMessageStorage.h>

// Schemas
#include <Schemas/PingPong.h>

PingPongListener::PingPongListener(IBridge *owner) : bridge(owner) {

}

void PingPongListener::Handle(const MessageStream *streams, uint32_t count) {
    MessageStream outgoing;
    MessageStreamView<PingPongMessage> outgoingView(outgoing);

    // Read incoming pings
    for (uint32_t i = 0; i < count; i++) {
        ConstMessageStreamView<PingPongMessage> view(streams[i]);

        // Pong all messages
        for (auto it = view.GetIterator(); it; ++it) {
            PingPongMessage* out = outgoingView.Add();
            out->timeStamp = it->timeStamp;
        }
    }

    // Add outgoing pongs
    bridge->GetOutput()->AddStream(outgoing);
}
