#pragma once

// Backend
#include "IShaderSGUIDHost.h"

// Bridge
#include <Bridge/IBridgeListener.h>

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Std
#include <unordered_map>
#include <vector>

// Forward declarations
class IShaderSGUIDHost;

class ShaderSGUIDHostListener : public TComponent<ShaderSGUIDHostListener>, public IBridgeListener {
public:
    COMPONENT(ShaderSGUIDHostListener);

    /// Get the mapping for a given sguid
    /// \param sguid
    /// \return
    ShaderSourceMapping GetMapping(ShaderSGUID sguid);

    /// Get the source code for a binding
    /// \param sguid the shader sguid
    /// \return the source code, empty if not found
    std::string_view GetSource(ShaderSGUID sguid);

    /// Overrides
    void Handle(const MessageStream *streams, uint32_t count) override;

private:
    struct Entry {
        ShaderSourceMapping mapping;
        std::string         contents;
    };

    /// Lookup
    std::vector<Entry> sguidLookup;
};
