#pragma once

namespace Bridge::CLR {
    public interface class IMessageStorage {
        /// Add a stream
        /// \param stream
        virtual void AddStream(Message::CLR::IMessageStream^ stream) = 0;
    };
}
