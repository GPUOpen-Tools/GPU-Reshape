#pragma once

// Std
#include <set>

// Xml
#include <tinyxml2.h>

// Generator
#include <Common/TemplateEngine.h>

struct GeneratorInfo {
    /// Specification registry node
    tinyxml2::XMLElement* schema{nullptr};

    /// Generated languages
    std::set<std::string> languages;
};

namespace Generators {
    /// Generate c++ sources
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool CPP(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate c# sources
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool CS(const GeneratorInfo& info, TemplateEngine& templateEngine);
}
