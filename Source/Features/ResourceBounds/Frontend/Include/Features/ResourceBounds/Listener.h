#pragma once

// Bridge
#include <Bridge/IBridgeListener.h>

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Forward declarations
class IBridge;
class IMessageHub;
class ShaderSGUIDHostListener;

class ResourceBoundsListener : public TComponent<ResourceBoundsListener>, public IBridgeListener {
public:
    COMPONENT(ResourceBoundsListener);

    /// Install this listener
    /// \return success state
    bool Install();

    /// Overrides
    void Handle(const MessageStream *streams, uint32_t count) override;

private:
    ComRef<IMessageHub> hub;
    ComRef<ShaderSGUIDHostListener> sguidHost;
};
