// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

#include "GenTypes.h"
#include "Types.h"

// Std
#include <iostream>

struct ProxiesState {
    /// All streams
    std::stringstream proxies;
};

/// Wrap a hooked class
static void WrapClass(const GeneratorInfo &info, ProxiesState &state, const std::string &key, const nlohmann::json &obj) {
    if (!obj.contains("proxies")) {
        return;
    }

    // Begin class
    state.proxies << "struct " << key << "FeatureProxies {\n";
    state.proxies << "\tCommandContext* context{nullptr};\n";

    // Create proxies
    for (const std::string proxy : obj["proxies"]) {
        // Generate feature bit set
        state.proxies << "\n\tuint64_t featureBitSet_" << proxy << "{0};\n";

        // Generate feature bit set mask
        state.proxies << "\tuint64_t featureBitSetMask_" << proxy << "{0};\n";

        // Generate feature callbacks
        state.proxies << "\tFeatureHook_" << proxy << "::Hook featureHooks_" << proxy << "[64];\n";
    }

    // End class
    state.proxies << "};\n\n";
}

bool Generators::FeatureProxies(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    ProxiesState state;

    // Common
    auto &&objects = info.hooks["objects"];

    // Wrap all hooked objects
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        WrapClass(info, state, it.key(), *it);
    }

    // Replace keys
    templateEngine.Substitute("$PROXIES", state.proxies.str().c_str());

    // OK
    return true;
}
