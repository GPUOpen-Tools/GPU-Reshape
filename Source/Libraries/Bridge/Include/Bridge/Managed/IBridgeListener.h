#pragma once

namespace Bridge::CLR {
    public interface class IBridgeListener {
        /// Handle a batch of streams
        /// \param streams the streams, length of [count]
        /// \param count the number of streams
        virtual void Handle(const Message::CLR::MessageStream^ streams, uint32_t count) = 0;
    };
}
