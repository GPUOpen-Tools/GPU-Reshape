// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
#include <vector>
#include <iostream>
#include <sstream>

struct TableState {
    /// All streams
    std::stringstream includes;
    std::stringstream tables;
    std::stringstream getters;
    std::stringstream detours;
    std::stringstream fwd;
};

/// Wrap a hooked class
static bool WrapClass(const GeneratorInfo &info, TableState &state, const std::string &key, const nlohmann::json &obj) {
    // Get the outer revision
    std::string outerRevision = GetOuterRevision(info, key);

    // Object common
    auto objName = obj["name"].get<std::string>();
    auto objState = obj["state"].get<std::string>();

    // Forward declaration
    state.fwd << "struct " << objState << ";\n";

    // Create table
    state.tables << "struct " << objName << "Table {\n";
    state.tables << "\t" << outerRevision << "DetourVTable *bottom{nullptr};\n";
    state.tables << "\t" << obj["state"].get<std::string>() << "* state{nullptr};\n";
    state.tables << "\t" << outerRevision << "* next{nullptr};\n";
    state.tables << "};\n\n";

    // Requested key may differ
    std::string consumerKey = obj.contains("type") ? obj["type"].get<std::string>() : key;

    // Getter prototype
    state.getters << objName << "Table GetTable(" << consumerKey << "* object);\n";

    // Detour prototype
    state.detours << consumerKey << "* CreateDetour(const Allocators& allocators, " << consumerKey << "* object, " << obj["state"].get<std::string>() << "* state);\n";

    // OK
    return true;
}

bool Generators::Table(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    TableState state;

    // Print includes
    if (info.hooks.contains("includes")) {
        for (auto &&include: info.hooks["includes"]) {
            state.includes << "#include <Backends/DX12/" << include.get<std::string>() << ">\n";
        }
    }

    // Common
    auto &&objects = info.hooks["objects"];

    // Wrap all hooked classes
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if (!WrapClass(info, state, it.key(), *it)) {
            return false;
        }
    }

    // Replace keys
    templateEngine.Substitute("$INCLUDES", state.includes.str().c_str());
    templateEngine.Substitute("$FWD", state.fwd.str().c_str());
    templateEngine.Substitute("$TABLES", state.tables.str().c_str());
    templateEngine.Substitute("$GETTERS", state.getters.str().c_str());
    templateEngine.Substitute("$DETOURS", state.detours.str().c_str());

    // OK
    return true;
}
