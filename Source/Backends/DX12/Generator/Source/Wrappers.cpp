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
#include <vector>
#include <iostream>
#include <sstream>

struct WrapperState {
    /// All streams
    std::stringstream wrap;
    std::stringstream hooks;
    std::stringstream fwd;
};

/// Wrap a given class interface
static bool WrapClassInterface(const GeneratorInfo &info, WrapperState& state, const std::string& key, const std::string& consumerKey, const nlohmann::json& obj) {
    // For all bases
    for (auto&& base : obj["bases"]) {
        auto&& baseInterface = info.specification["interfaces"][base.get<std::string>()];

        // Detour base
        if (!WrapClassInterface(info, state, key, consumerKey, baseInterface)) {
            return false;
        }
    }

    // Generate hooks prototypes
    for (auto&& method : obj["vtable"]) {
        // Common
        auto methodName = method["name"].get<std::string>();

        // Get contained (fptr) type
        auto &&parameters = method["params"];

        bool isStructRet = IsTypeStruct(method["returnType"]);

        // Structured?
        if (!isStructRet) {
            // Print return type
            if (!PrettyPrintType(state.hooks, method["returnType"])) {
                return false;
            }
        } else {
            state.hooks << "void";
        }

        // Print name of wrapper
        state.hooks << " Wrapper_Hook" << consumerKey << methodName << "(";

        // Detour initial argument to wrapper
        state.hooks << key << "Wrapper* _this";

        // Print standard arguments
        for (size_t i = 0; i < parameters.size(); i++) {
            auto&& param = parameters[i];

            state.hooks << ", ";

            // Dedicated printer
            if (!PrettyPrintParameter(state.hooks, param["type"], param["name"].get<std::string>())) {
                return false;
            }
        }

        // Structured parameter
        if (isStructRet) {
            state.hooks << ", ";

            if (!PrettyPrintType(state.hooks, method["returnType"])) {
                return false;
            }

            state.hooks << "* rdx";
        }

        // End wrapper
        state.hooks << ");\n";
    }

    // OK
    return true;
}

/// Wrap a hooked object
static bool WrapClass(const GeneratorInfo &info, WrapperState& state, const std::string& key, const nlohmann::json& obj) {
    // Get the outer revision
    std::string outerRevision = GetOuterRevision(info, key);

    // Common
    auto&& interfaces = info.specification["interfaces"];

    // Get the vtable
    auto&& objInterface = interfaces[outerRevision];

    // Forward declarations
    state.fwd << "struct " << obj["state"].get<std::string>() << ";\n";

    // Generate wrapper type
    state.wrap << "struct " << key << "Wrapper {\n";
    state.wrap << "\t" << key << "Wrapper();\n\n";
    state.wrap << "\tstatic " << outerRevision << "DetourVTable topImage;\n";
    state.wrap << "\t" << outerRevision << "DetourVTable* top = &topImage;\n";
    state.wrap << "\t" << outerRevision << "* next;\n";
    state.wrap << "\t" << obj["state"].get<std::string>() << "* state;\n";
    state.wrap << "};\n\n";

    // Requested key may differ
    std::string consumerKey = obj.contains("type") ? obj["type"].get<std::string>() : key;

    // Wrap interface
    return WrapClassInterface(info, state, key, consumerKey, objInterface);
}

bool Generators::Wrappers(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    WrapperState state;

    // Common
    auto&& objects = info.hooks["objects"];

    // Wrap all hooked objects
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if (!WrapClass(info, state, it.key(), *it)) {
            return false;
        }
    }

    // Replace keys
    templateEngine.Substitute("$FWD", state.fwd.str().c_str());
    templateEngine.Substitute("$WRAPPERS", state.wrap.str().c_str());
    templateEngine.Substitute("$HOOKS", state.hooks.str().c_str());

    // OK
    return true;
}
