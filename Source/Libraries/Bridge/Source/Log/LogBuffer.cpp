#include <Bridge/Log/LogBuffer.h>
#include <Bridge/IBridge.h>

// Message
#include <Message/IMessageStorage.h>

// Schemas
#include <Schemas/Log.h>

void LogBuffer::Add(const std::string_view& system, const std::string_view& message) {
    std::lock_guard guard(mutex);

    MessageStreamView<LogMessage> view(stream);

    LogMessage* log = view.Add(LogMessage::AllocationInfo {
        .systemLength = system.length(),
        .messageLength = message.length()
    });

    log->system.Set(system);
    log->message.Set(message);
}

void LogBuffer::Commit(IBridge *bridge) {
    std::lock_guard guard(mutex);

    bridge->GetOutput()->AddStreamAndSwap(stream);
}
