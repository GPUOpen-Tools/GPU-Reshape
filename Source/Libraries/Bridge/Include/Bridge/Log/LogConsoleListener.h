#pragma once

// Bridge
#include <Bridge/IBridgeListener.h>

// Common
#include <Common/IComponent.h>

/// Log to console redirector
class LogConsoleListener : public TComponent<LogConsoleListener>, public IBridgeListener {
public:
    COMPONENT(LogConsoleListener);

    void Handle(const MessageStream *streams, uint32_t count) override;
};
