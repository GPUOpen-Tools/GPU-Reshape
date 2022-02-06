#include <Backend/ShaderSGUIDHostListener.h>
#include <Backend/IShaderSGUIDHost.h>

// Common
#include <Common/Registry.h>

// Message
#include <Message/MessageStream.h>

// Schemas
#include <Schemas/SGUID.h>

// Std
#include <algorithm>

void ShaderSGUIDHostListener::Handle(const MessageStream *streams, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        ConstMessageStreamView<ShaderSourceMappingMessage> view(streams[i]);

        // Determine bound
        uint32_t sguidBound{0};
        for (auto it = view.GetIterator(); it; ++it) {
            sguidBound = std::max(sguidBound, it->sguid);
        }

        // Ensure enough space
        if (sguidBound >= sguidLookup.size()) {
            sguidLookup.resize(sguidBound + 1);
        }

        // Populate entries
        for (auto it = view.GetIterator(); it; ++it) {
            Entry& entry = sguidLookup.at(it->sguid);
            entry.mapping.shaderGUID = it->shaderGUID;
            entry.mapping.fileUID = it->fileUID;
            entry.mapping.line = it->line;
            entry.mapping.column = it->column;
            entry.contents = std::string(it->contents.View());
        }
    }
}

ShaderSourceMapping ShaderSGUIDHostListener::GetMapping(ShaderSGUID sguid) {
    return sguidLookup.at(sguid).mapping;
}

std::string_view ShaderSGUIDHostListener::GetSource(ShaderSGUID sguid) {
    return sguidLookup.at(sguid).contents;
}
