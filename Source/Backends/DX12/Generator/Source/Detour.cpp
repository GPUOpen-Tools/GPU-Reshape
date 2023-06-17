// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
#include <set>

struct DetourState {
    /// Current set of mapped functions
    std::set<std::string> functions;

    /// Current set of mapped interfaces
    std::set<std::string> interfaces;

    /// All streams
    std::stringstream includes;
    std::stringstream pfn;
    std::stringstream tables;
    std::stringstream offsets;
    std::stringstream typedefs;
    std::stringstream populators;
};

/// Detour a given interface
static bool DetourInterface(const GeneratorInfo &info, DetourState& state, const std::string& key, const nlohmann::json& interface) {
    // For all bases
    for (auto&& base : interface["bases"]) {
        auto baseKey = base.get<std::string>();

        auto&& baseInterface = info.specification["interfaces"][baseKey];

        // Detour base
        if (!DetourInterface(info, state, baseKey, baseInterface)) {
            return false;
        }
    }

    // For all vtable fields
    for (auto&& method : interface["vtable"]) {
        // Get name of function
        auto fieldName = method["name"].get<std::string>();

        // To function type
        std::string pfnName = "PFN_" + key + fieldName;

        // Emit PFN once
        if (!state.functions.count(pfnName)) {
            state.pfn << "using " << pfnName << " = ";

            bool isStructRet = IsTypeStruct(method["returnType"]);

            // Structured?
            if (!isStructRet) {
                if (!PrettyPrintType(state.pfn, method["returnType"])) {
                    return false;
                }
            } else {
                state.pfn << "void";
            }

            state.pfn << "(*)(";
            state.pfn << key << "* _this";

            for (auto&& param : method["params"]) {
                state.pfn << ", ";

                if (!PrettyPrintParameter(state.pfn, param["type"], param["name"].get<std::string>())) {
                    return false;
                }
            }

            // Structured argument
            if (isStructRet) {
                state.pfn << ", ";

                if (!PrettyPrintType(state.pfn, method["returnType"])) {
                    return false;
                }

                state.pfn << "* rdx";
            }

            state.pfn << ");\n";
            state.functions.insert(pfnName);
        }

        // Append table and offset members
        state.tables << "\t" << "" << pfnName << " next_" << fieldName << ";\n";
        state.offsets << "\t" << fieldName << ",\n";
    }

    // OK
    return true;
}

/// Detour a given interface
static bool DetourObject(const GeneratorInfo &info, DetourState& state, const std::string& key) {
    // Already detoured?
    if (state.interfaces.count(key)) {
        return true;
    }

    // Append
    state.interfaces.insert(key);

    // Get interface
    auto&& interface = info.specification["interfaces"][key];

    // Detour all bases
    for (auto&& base : interface["bases"]) {
        DetourObject(info, state, base.get<std::string>());
    }

    // Emit types
    state.tables << "struct " << key << "DetourVTable {\n";
    state.offsets << "enum class " << key << "DetourOffsets : uint32_t {\n";

    // Generate contents
    if (!DetourInterface(info, state, key, interface)) {
        return false;
    }

    state.tables << "};\n\n";
    state.offsets << "};\n\n";

    return true;
}

static void DetourBaseQuery(const GeneratorInfo &info, DetourState& state, const std::string& key, bool top = true) {
    auto&& obj = info.specification["interfaces"][key];

    // Revision table name
    std::string vtblName = key + "DetourVTable";

    // Keep it clean
    if (!top) {
        state.populators << " else ";
    }
    // Copy vtable contents
    state.populators << "if (SUCCEEDED(object->QueryInterface(__uuidof(" << key << "), &_interface))) {\n";
    state.populators << "\t\tstd::memcpy(&out, *(" << vtblName << "**)_interface, sizeof(" << vtblName << "));\n";
    state.populators << "\t\tobject->Release();\n";
    state.populators << "\t}";

    // Detour all bases
    for (auto&& base : obj["bases"]) {
        DetourBaseQuery(info, state, base.get<std::string>(), false);
    }
}

bool Generators::Detour(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    DetourState state;

    // Print includes
    for (auto&& include : info.hooks["files"]) {
        state.includes << "#include <" << include.get<std::string>() << ">\n";
    }

    // Common
    auto&& objects    = info.hooks["objects"];

    // Generate detours for all hooked objects
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        std::string key = it.key();

        // Get outer revision
        std::string outerRevision = GetOuterRevision(info, key);

        // Detour outer
        if (!DetourObject(info, state, outerRevision)) {
            return false;
        }

        // Typedef
        state.typedefs << "using " << key << "TopDetourVTable = " << outerRevision << "DetourVTable;\n";

        // Populators
        state.populators << "static inline " << key << "TopDetourVTable PopulateTopDetourVTable(" << key << "* object) {\n";
        state.populators << "\t" << key << "TopDetourVTable out{};\n";
        state.populators << "\n";
        state.populators << "\tvoid* _interface;\n";

        // Detour all bases
        state.populators << "\t";
        DetourBaseQuery(info, state, outerRevision);

        // DOne
        state.populators << "\n";
        state.populators << "\treturn out;\n";
        state.populators << "}\n\n";
    }

    // Replace keys
    templateEngine.Substitute("$INCLUDES", state.includes.str().c_str());
    templateEngine.Substitute("$PFN", state.pfn.str().c_str());
    templateEngine.Substitute("$TABLES", state.tables.str().c_str());
    templateEngine.Substitute("$OFFSETS", state.offsets.str().c_str());
    templateEngine.Substitute("$TYPEDEFS", state.typedefs.str().c_str());
    templateEngine.Substitute("$POPULATORS", state.populators.str().c_str());

    // OK
    return true;
}
