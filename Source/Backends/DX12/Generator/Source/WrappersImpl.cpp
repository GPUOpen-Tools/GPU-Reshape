#include "GenTypes.h"
#include "Types.h"

// Std
#include <vector>
#include <iostream>
#include <sstream>

struct WrapperImplState {
    /// All streams
    std::stringstream includes;
    std::stringstream ss;
    std::stringstream tbl;
    std::stringstream hooks;
    std::stringstream detours;
    std::stringstream getters;
};

/// Wrap a hooked class
static bool WrapClass(const GeneratorInfo &info, WrapperImplState &state, const std::string &key, const nlohmann::json &obj) {
    // Get the outer revision
    std::string outerRevision = GetOuterRevision(info, key);

    // Common
    auto &&hooks = obj["hooks"];
    auto &&interfaces = info.specification["interfaces"];

    // Get vtable
    auto &&objInterface = interfaces[outerRevision];
    auto &&objVtbl = interfaces[std::string(objInterface["vtable"])];

    // Object common
    auto name = obj["name"].get<std::string>();

    // Begin top images
    state.tbl << outerRevision << "DetourVTable " << key << "Wrapper::topImage = {\n";

    // Generate top image writers
    for (auto &&field: objVtbl["fields"]) {
        auto &&type = field["type"];

        // Skip non function pointers
        if (type["type"] != "pointer" && type["contained"]["type"] != "function") {
            continue;
        }

        // Common
        auto fieldName = field["name"].get<std::string>();

        // Set wrapper callback
        state.tbl << "\t.next_" << fieldName << " = reinterpret_cast<decltype(" << outerRevision << "DetourVTable::next_" << fieldName << ")>(Wrapper_Hook" << key << fieldName << "),\n";
    }

    // End top images
    state.tbl << "};\n\n";

    // Placeholder wrapper constructor
    state.ss << key << "Wrapper::" << key << "Wrapper() {\n";
    state.ss << "\t/* poof */\n";
    state.ss << "}\n";

    // Generate wrappers
    for (auto &&field: objVtbl["fields"]) {
        auto &&type = field["type"];

        // Common
        auto fieldName = field["name"].get<std::string>();

        // Skip non function pointers
        if (type["type"] != "pointer" && type["contained"]["type"] != "function") {
            continue;
        }

        // Get contained (fptr) type
        auto &&funcType = type["contained"];
        auto &&parameters = funcType["parameters"];

        // Print return type
        if (!PrettyPrintType(state.hooks, funcType["returnType"])) {
            return false;
        }

        // Print name of wrapper
        state.hooks << " Wrapper_Hook" << key << fieldName << "(";

        // Detour initial argument to wrapper
        state.hooks << key << "Wrapper* _this";

        // Print standard arguments
        for (size_t i = 1; i < parameters.size(); i++) {
            state.hooks << ", ";

            // Dedicated printer
            if (!PrettyPrintParameter(state.hooks, parameters[i], "_" + std::to_string(i))) {
                return false;
            }
        }

        // Begin body
        state.hooks << ") {\n";

        // Check if this wrapper is hooked
        bool isHooked{false};
        for (auto hook: hooks) {
            isHooked |= hook == fieldName;
        }

        // Hooked?
        if (isHooked) {
            // Print return if needed
            if (funcType["returnType"]["type"] == "void") {
                state.hooks << "\t";
            } else {
                state.hooks << "\treturn ";
            }

            // Proxy down to hook
            state.hooks << "Hook" << key << fieldName << "(reinterpret_cast<";

            // Print wrapped type
            if (!PrettyPrintType(state.hooks, parameters[0])) {
                return false;
            }

            state.hooks << ">(_this)";

            // Forward parameters
            for (size_t i = 1; i < parameters.size(); i++) {
                state.hooks << ", _" << i;
            }
        } else {
            // Special generator for interface querying
            if (fieldName == "QueryInterface") {
                // If unwrapping, simply proxy down the next object
                state.hooks << "\tif (*_1 == IID_Unwrap) {\n";
                state.hooks << "\t\t/* No ref added */\n";
                state.hooks << "\t\t*_2 = _this->next;\n";
                state.hooks << "\t\treturn S_OK;\n";
                state.hooks << "\t}\n\n";

                // Base revision?
                state.hooks << "\tif (*_1 == __uuidof(" << key << ")) {\n";
                state.hooks << "\t\t_this->next->AddRef();\n";
                state.hooks << "\t\t*_2 = reinterpret_cast<" << key << "*>(_this);\n";
                state.hooks << "\t\treturn S_OK;\n";
                state.hooks << "\t}";

                // Extended revisions?
                for (uint32_t i = 1; info.specification["interfaces"].contains(key + std::to_string(i)); i++) {
                    state.hooks << " else if (*_1 == __uuidof(" << key << i << ")) {\n";
                    state.hooks << "\t\t_this->next->AddRef();\n";
                    state.hooks << "\t\t*_2 = reinterpret_cast<" << key << i << "*>(_this);\n";
                    state.hooks << "\t\treturn S_OK;\n";
                    state.hooks << "\t}";
                }

                // Unknown interface, pass down to next object
                state.hooks << "\n\n\treturn GetVTableRaw<" << outerRevision << "DetourVTable>(_this->next)->next_" << fieldName << "(";
            } else {
                // Print return if needed
                if (funcType["returnType"]["type"] == "void") {
                    state.hooks << "\t";
                } else {
                    state.hooks << "\treturn ";
                }

                // Pass down to next object
                state.hooks << "GetVTableRaw<" << outerRevision << "DetourVTable>(_this->next)->next_" << fieldName << "(";
            }

            // Unwrap this
            state.hooks << "_this->next";

            // Unwrap arguments
            for (size_t i = 1; i < parameters.size(); i++) {
                state.hooks << ", Unwrap(_" << i << ")";
            }
        }

        // End call
        state.hooks << ");\n";

        // End function
        state.hooks << "}\n\n";
    }

    // Generate wrapper detouring
    state.detours << key << "* CreateDetour(const Allocators& allocators, " << key << "* object, " << obj["state"].get<std::string>() << "* state) {\n";
    state.detours << "\tauto* wrapper = new (allocators) " << key << "Wrapper();\n";
    state.detours << "\twrapper->next = static_cast<" << outerRevision << "*>(object);\n";
    state.detours << "\twrapper->state = state;\n";
    state.detours << "\treturn reinterpret_cast<" << key << "*>(wrapper);\n";
    state.detours << "}\n\n";

    // Generate table getter
    state.getters << name << "Table GetTable(" << key << "* object) {\n";
    state.getters << "\t" << name << "Table table;\n\n";
    state.getters << "\tauto wrapper = reinterpret_cast<" << key << "Wrapper*>(object);\n";
    state.getters << "\ttable.next = wrapper->next;\n";
    state.getters << "\ttable.bottom = GetVTableRaw<" << key << "TopDetourVTable>(wrapper->next);\n";
    state.getters << "\ttable.state = wrapper->state;\n";
    state.getters << "\treturn table;\n";
    state.getters << "}\n\n";

    // OK
    return true;
}

bool Generators::WrappersImpl(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    WrapperImplState state;

    // Print includes
    if (info.hooks.contains("includes")) {
        for (auto &&include: info.hooks["includes"]) {
            state.includes << "#include <Backends/DX12/" << include.get<std::string>() << ">\n";
        }
    }

    // Common
    auto &&objects = info.hooks["objects"];

    // Wrap all hooked objects
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        if (!WrapClass(info, state, it.key(), *it)) {
            return false;
        }
    }

    // Replace keys
    templateEngine.Substitute("$INCLUDES", state.includes.str().c_str());
    templateEngine.Substitute("$IMPL", state.ss.str().c_str());
    templateEngine.Substitute("$TABLE", state.tbl.str().c_str());
    templateEngine.Substitute("$HOOKS", state.hooks.str().c_str());
    templateEngine.Substitute("$DETOURS", state.detours.str().c_str());
    templateEngine.Substitute("$GETTERS", state.getters.str().c_str());

    // OK
    return true;
}
