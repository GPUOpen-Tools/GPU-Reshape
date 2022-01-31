#include <Bridge/Log/LogConsoleListener.h>

// Message
#include <Message/MessageStream.h>

// Schemas
#include <Schemas/Log.h>

// Std
#include <iostream>

void LogConsoleListener::Handle(const MessageStream *streams, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        ConstMessageStreamView<LogMessage> view(streams[i]);

        for (auto it = view.GetIterator(); it; ++it) {
            std::cout << "[" << (it->system.Empty() ? "None" : it->system.View()) << "] " << it->message.View() << "\n";
        }

        std::cout.flush();
    }
}
