#pragma once

// Bridge
#include <Bridge/IBridgeListener.h>

// Common
#include <Common/IComponent.h>

/// Log to console redirector
class LogConsoleListener : public IComponent, public IBridgeListener {
public:
    void Handle(const MessageStream *streams, uint32_t count) override;
};
