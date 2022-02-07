#pragma once

// Bridge
#include <Bridge/IBridgeListener.h>

// Common
#include <Common/IComponent.h>

// Forward declarations
class IBridge;

class PingPongListener : public TComponent<PingPongListener>, public IBridgeListener {
public:
    COMPONENT(PingPongListener);

    /// Constructor
    /// \param owner unsafe owner, cannot be ref due to cyclic references
    PingPongListener(IBridge* owner);

    /// Overrides
    void Handle(const MessageStream *streams, uint32_t count) override;

private:
    IBridge* bridge{nullptr};
};
