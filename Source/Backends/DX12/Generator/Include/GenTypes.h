// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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
