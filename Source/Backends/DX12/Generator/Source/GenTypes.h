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
    /// Optional spec json
    nlohmann::json specification{};

    /// Optional hooks json
    nlohmann::json hooks{};

    /// D3D12 header path
    std::string d3d12HeaderPath;
};

namespace Generators {
    /// Generate the specification
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool Specification(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the specification
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool Detour(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the specification
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool Wrappers(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the specification
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool WrappersImpl(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the specification
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool VTable(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the specification
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool Table(const GeneratorInfo& info, TemplateEngine& templateEngine);
}
