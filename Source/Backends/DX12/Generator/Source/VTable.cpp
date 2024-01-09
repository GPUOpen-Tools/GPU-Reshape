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

#include "GenTypes.h"
#include "Types.h"

// Std
#include <iostream>

struct VTableState {
    /// All streams
    std::stringstream includes;
    std::stringstream detours;
    std::stringstream getters;
};

/// Wrap a hooked class
static bool WrapClass(const GeneratorInfo &info, VTableState& state, const std::string& key, const nlohmann::json& obj) {
    // Get the outer revision
    std::string outerRevision = GetOuterRevision(info, key);

    // Object common
    auto&& hooks    = obj["hooks"];
    auto   objName  = obj["name"].get<std::string>();
    auto   objState = obj["state"].get<std::string>();

    // Begin detours
    state.detours << key << "* CreateDetour(const Allocators& allocators, " << key << "* object, " << objState << "* state) {\n";
    state.detours << "\tauto vtable = DetourVTable<" << key << ", " << obj["state"].get<std::string>() << ">::NewAndPatch(allocators, object);\n";

    // Proxy all hooked functions
    for (auto&& hook : hooks) {
        auto hookName = hook.get<std::string>();
        state.detours << "\tvtable->top.next_" << hookName << " = Hook" << key << hookName << ";\n";
    }

    // End detours
    state.detours << "\tvtable->object = state;\n";
    state.detours << "\treturn object;\n";
    state.detours << "}\n\n";

    // Table getter
    state.getters << objName << "Table GetTable(" << key << "* object) {\n";
    state.getters << "\t" << objName << "Table table;\n\n";
    state.getters << "\tauto vtable = DetourVTable<" << key << ", " << obj["state"].get<std::string>() << ">::Get(object);\n";
    state.getters << "\ttable.next = static_cast<" << outerRevision << "*>(object);\n";
    state.getters << "\ttable.bottom = &vtable->bottom;\n";
    state.getters << "\ttable.state = vtable->object;\n";
    state.getters << "\treturn table;\n";
    state.getters << "}\n\n";

    return true;
}

bool Generators::VTable(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    VTableState state;

    // Print includes
    if (info.hooks.contains("includes")) {
        for (auto&& include : info.hooks["includes"]) {
            state.includes << "#include <Backends/DX12/" << include.get<std::string>() << ">\n";
        }
    }

    // Common
    auto&& objects = info.hooks["objects"];

    // Wrap all hooked objects
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if (!WrapClass(info, state, it.key(), *it)) {
            return false;
        }
    }

    // Replace keys
    templateEngine.Substitute("$INCLUDES", state.includes.str().c_str());
    templateEngine.Substitute("$DETOURS", state.detours.str().c_str());
    templateEngine.Substitute("$GETTERS", state.getters.str().c_str());

    // OK
    return true;
}
