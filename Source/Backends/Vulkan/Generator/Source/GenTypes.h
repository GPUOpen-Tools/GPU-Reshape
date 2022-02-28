#pragma once

// Std
#include <set>

// Xml
#include <tinyxml2.h>

// Json
#include <nlohmann/json.hpp>

// Common
#include <Common/TemplateEngine.h>

struct GeneratorInfo {
    /// Specification registry node
    tinyxml2::XMLElement* registry{nullptr};

    /// Optional spirv json
    nlohmann::json spvJson{};

    /// Whitelisted commands, context sensitive
    std::set<std::string> whitelist;

    /// Objects, context sensitive
    std::set<std::string> objects;

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

    /// Generate the deep copy object implementation
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool DeepCopyObjects(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the deep copy implementation
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool DeepCopy(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate spv helpers
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool Spv(const GeneratorInfo& info, TemplateEngine& templateEngine);
}
