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

    /// Optional hooks json
    nlohmann::json deepCopy{};

    /// Optional DXIL rst
    std::string dxilRST;

    /// D3D12 header path
    std::string d3d12HeaderPath;
};

namespace Generators {
    /// Generate the specification
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool Specification(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the detours
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool Detour(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the wrappers
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool Wrappers(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the wrapper implementation
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool WrappersImpl(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the object wrapper implementation
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool ObjectWrappers(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the virtual table
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool VTable(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the table
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool Table(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the deep copy
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool DeepCopy(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the deep copy implementation
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool DeepCopyImpl(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the DXIL header
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool DXILTables(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the DXIL intrinsics
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool DXILIntrinsics(const GeneratorInfo& info, TemplateEngine& templateEngine);

    /// Generate the feature proxies
    /// \param info generation info
    /// \param templateEngine template destination
    /// \return success
    bool FeatureProxies(const GeneratorInfo& info, TemplateEngine& templateEngine);
}
