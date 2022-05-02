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

    /// Basename to interface revisions
    std::map<std::string, uint32_t> interfaceRevisions;

    /// All streams
    std::stringstream includes;
    std::stringstream pfn;
    std::stringstream tables;
    std::stringstream offsets;
    std::stringstream typedefs;
    std::stringstream populators;
};

/// Detour a given interface
static bool DetourInterface(const GeneratorInfo &info, DetourState& state, const std::string& key, const std::string& name) {
    auto&& vtable = info.specification["interfaces"][name];

    // For all vtable fields
    for (auto&& field : vtable["fields"]) {
        auto&& type = field["type"];

        // Skip non function pointers
        if (type["type"] != "pointer" && type["contained"]["type"] != "function") {
            continue;
        }

        // Get name of function
        auto fieldName = field["name"].get<std::string>();

        // To function type
        std::string pfnName = "PFN_" + GetInterfaceBaseName(key) + fieldName;

        // Skip generation of PFN if basename is already present
        if (!state.functions.contains(pfnName)) {
            auto&& function = type["contained"];
            state.pfn << "using " << pfnName << " = ";

            // Print fptr type
            if (!PrettyPrintType(state.pfn, type)) {
                return false;
            }

            state.pfn << ";\n";
            state.functions.insert(pfnName);
        }

        // Append table and offset members
        state.tables << "\t" << "" << pfnName << " next_" << fieldName << ";\n";
        state.offsets << "\t" << fieldName << ",\n";
    }

    // OK
    return true;
}

bool Generators::Detour(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    DetourState state;

    // Print includes
    for (auto&& include : info.hooks["files"]) {
        state.includes << "#include <" << include.get<std::string>() << ">\n";
    }

    // Common
    auto&& interfaces = info.specification["interfaces"];
    auto&& objects    = info.hooks["objects"];

    // Generate detours for all hooked objects
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        // Keep going while there's another interface
        for (uint32_t revision = 0; interfaces.contains(GetInterfaceRevision(it.key(), revision)); revision++) {
            // Get revision name
            std::string key = GetInterfaceRevision(it.key(), revision);

            // Get interface
            auto interface = interfaces[key];
            if (!interface.contains("vtable")) {
                std::cerr << "Interface " << key << " has no virtual table\n";
                continue;
            }

            // Emit types
            state.tables << "struct " << key << "DetourVTable {\n";
            state.offsets << "enum class " << key << "DetourOffsets : uint32_t {\n";

            // Increment interface version
            state.interfaceRevisions[it.key()]++;

            // Generate contents
            if (!DetourInterface(info, state, key, std::string(interface["vtable"]))) {
                return false;
            }

            state.tables << "};\n\n";
            state.offsets << "};\n\n";
        }
    }

    // Typedefs
    for (auto&& kv : state.interfaceRevisions) {
        if (kv.second > 1) {
            state.typedefs << "using " << kv.first << "TopDetourVTable = " << kv.first << (kv.second - 1) << "DetourVTable;\n";
        } else {
            state.typedefs << "using " << kv.first << "TopDetourVTable = " << kv.first << "DetourVTable;\n";
        }
    }

    // Populators
    for (auto&& kv : state.interfaceRevisions) {
        state.populators << "static " << kv.first << "TopDetourVTable PopulateTopDetourVTable(" << kv.first << "* object) {\n";
        state.populators << "\t" << kv.first << "TopDetourVTable out{};\n";

        // Any revisions at all?
        if (kv.second > 1) {
            state.populators << "\n";
            state.populators << "\tvoid* _interface;\n";

            // Generate fetchers
            state.populators << "\t";
            for (int32_t i = static_cast<int32_t>(kv.second) - 1; i >= 1; i--) {
                std::string objName;
                if (i > 0) {
                    objName = kv.first + std::to_string(i);
                } else {
                    objName = kv.first;
                }

                // Revision table name
                std::string vtblName = objName + "DetourVTable";

                if (i != kv.second - 1) {
                    state.populators << "else ";
                }

                // Copy vtable contents
                state.populators << "if (SUCCEEDED(object->QueryInterface(__uuidof(" << objName << "), &_interface))) {\n";
                state.populators << "\t\tstd::memcpy(&out, *(" << vtblName << "**)_interface, sizeof(" << vtblName << "));\n";
                state.populators << "\t\tobject->Release();\n";
                state.populators << "\t} ";
            }

            // Base revision
            state.populators << "else {\n";
            state.populators << "\t\tstd::memcpy(&out, *(" << kv.first << "DetourVTable**)object, sizeof(" << kv.first << "DetourVTable));\n";
            state.populators << "\t}\n";
        } else {
            // Just copy the base revision
            state.populators << "\tstd::memcpy(&out, *(" << kv.first << "DetourVTable**)object, sizeof(" << kv.first << "DetourVTable));\n";
        }

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
