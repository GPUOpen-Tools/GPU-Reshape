#pragma once

namespace Bridge::CLR {
    public interface class IMessageStorage {
        /// Add a stream
        /// \param stream
        virtual void AddStream(const Message::CLR::MessageStream^ stream) = 0;
    };
}
