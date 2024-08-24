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

// Generator
#include "Filter.h"

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

    /// Filtered extension info
    FilterInfo filter;

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


