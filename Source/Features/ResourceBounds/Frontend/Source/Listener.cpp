#include <Features/ResourceBounds/Listener.h>

// Message
#include <Message/IMessageHub.h>

// Backend
#include <Backend/ShaderSGUIDHostListener.h>

// Schemas
#include <Schemas/Features/ResourceBounds.h>

// Common
#include <Common/Registry.h>
#include <Common/Format.h>

bool ResourceBoundsListener::Install() {
    hub = registry->Get<IMessageHub>();
    if (!hub) {
        return false;
    }

    // Host is optional
    sguidHost = registry->Get<ShaderSGUIDHostListener>();

    // OK
    return true;
}

void ResourceBoundsListener::Handle(const MessageStream *streams, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        ConstMessageStreamView<ResourceIndexOutOfBoundsMessage> view(streams[i]);

        // Foreach message
        for (auto it = view.GetIterator(); it; ++it) {
            std::string_view source = "";

            // Get source code
            if (sguidHost && it->sguid != InvalidShaderSGUID) {
                source = sguidHost->GetSource(it->sguid);
            }

            // Compose message to hug
            hub->Add("ResourceIndexOutOfBounds", Format("{} {} out of bounds\n\t{}", it->isTexture ? "texture" : "buffer", it->isWrite ? "write" : "read", source));
        }
    }
}
