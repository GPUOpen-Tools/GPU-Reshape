#pragma once

// Std
#include <set>

// Xml
#include <tinyxml2.h>

// Generator
#include "TemplateEngine.h"

struct GeneratorInfo {
    /// Specification registry node
    tinyxml2::XMLElement* registry{nullptr};

    /// Whitelisted commands, context sensitive
    std::set<std::string> whitelist;

    /// Hooked commands, context sensitive
    std::set<std::string> hooks;
};

namespace Generators {
    /// Generate the command buffer implementation
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool CommandBuffer(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the command buffer dispatch table implementation
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool CommandBufferDispatchTable(const GeneratorInfo& info, TemplateEngine& templateEngine);
}
