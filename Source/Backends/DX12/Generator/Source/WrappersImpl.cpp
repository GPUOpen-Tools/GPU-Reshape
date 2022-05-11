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
static void WrapClassTopImage(const GeneratorInfo &info, WrapperImplState &state, const std::string &key, const std::string& consumerKey, const std::string& outerRevision, const nlohmann::json &obj) {
    // Common
    auto &&objVtbl = obj["vtable"];

    // For all bases
    for (auto&& base : obj["bases"]) {
        auto&& baseInterface = info.specification["interfaces"][base.get<std::string>()];

        // Wrap base image
        WrapClassTopImage(info, state, key, consumerKey, outerRevision, baseInterface);
    }

    // Generate top image writers
    for (auto &&method: objVtbl) {
        // Common
        auto methodName = method["name"].get<std::string>();

        // Set wrapper callback
        state.tbl << "\t.next_" << methodName << " = reinterpret_cast<decltype(" << outerRevision << "DetourVTable::next_" << methodName << ")>(Wrapper_Hook" << consumerKey << methodName << "),\n";
    }
}

/// Wrap the base interface queries
static void WrapClassBaseQuery(const GeneratorInfo &info, WrapperImplState& state, const std::string& key, bool top = true) {
    auto&& obj = info.specification["interfaces"][key];

    // Keep it clean
    if (!top) {
        state.hooks << " else ";
    }

    // Query check
    state.hooks << "if (riid == __uuidof(" << key << ")) {\n";
    state.hooks << "\t\t_this->next->AddRef();\n";
    state.hooks << "\t\t*ppvObject = reinterpret_cast<" << key << "*>(_this);\n";
    state.hooks << "\t\treturn S_OK;\n";
    state.hooks << "\t}";

    // Query all bases
    for (auto&& base : obj["bases"]) {
        WrapClassBaseQuery(info, state, base.get<std::string>(), false);
    }
}

static bool WrapClassMethods(const GeneratorInfo &info, WrapperImplState &state, const std::string &key, const std::string& consumerKey, const nlohmann::json &hooks, const nlohmann::json &obj) {
    // For all bases
    for (auto&& base : obj["bases"]) {
        auto&& baseInterface = info.specification["interfaces"][base.get<std::string>()];

        // Wrap base image
        WrapClassMethods(info, state, key, consumerKey, hooks, baseInterface);
    }

    // Get the outer revision
    std::string outerRevision = GetOuterRevision(info, key);

    // Generate wrappers
    for (auto &&method: obj["vtable"]) {
        // Common
        auto methodName = method["name"].get<std::string>();

        // Get parameters
        auto &&parameters = method["params"];

        // Print return type
        if (!PrettyPrintType(state.hooks, method["returnType"])) {
            return false;
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

        // Begin body
        state.hooks << ") {\n";

        // Check if this wrapper is hooked
        bool isHooked{false};
        for (auto hook: hooks) {
            isHooked |= hook.get<std::string>() == methodName;
        }

        // Hooked?
        if (isHooked) {
            // Print return if needed
            if (method["returnType"]["type"] == "void") {
                state.hooks << "\t";
            } else {
                state.hooks << "\treturn ";
            }

            // Proxy down to hook
            state.hooks << "Hook" << consumerKey << methodName << "(reinterpret_cast<" << outerRevision << "*>(_this)";

            // Forward parameters
            for (size_t i = 0; i < parameters.size(); i++) {
                state.hooks << ", " << parameters[i]["name"].get<std::string>();
            }
        } else {
            // Special generator for interface querying
            if (methodName == "QueryInterface") {
                // If unwrapping, simply proxy down the next object
                state.hooks << "\tif (riid == IID_Unwrap) {\n";
                state.hooks << "\t\t/* No ref added */\n";
                state.hooks << "\t\t*ppvObject = _this->next;\n";
                state.hooks << "\t\treturn S_OK;\n";
                state.hooks << "\t}\n\n";

                // Wrap query
                state.hooks << "\t";
                WrapClassBaseQuery(info, state, outerRevision);

                // Unknown interface, pass down to next object
                state.hooks << "\n\n\treturn GetVTableRaw<" << outerRevision << "DetourVTable>(_this->next)->next_" << methodName << "(";
            } else {
                // Print return if needed
                if (method["returnType"]["type"] == "void") {
                    state.hooks << "\t";
                } else {
                    state.hooks << "\treturn ";
                }

                // Pass down to next object
                state.hooks << "GetVTableRaw<" << outerRevision << "DetourVTable>(_this->next)->next_" << methodName << "(";
            }

            // Unwrap this
            state.hooks << "_this->next";

            // Unwrap arguments
            for (size_t i = 0; i < parameters.size(); i++) {
                state.hooks << ", Unwrap(" << parameters[i]["name"].get<std::string>() << ")";
            }
        }

        // End call
        state.hooks << ");\n";

        // End function
        state.hooks << "}\n\n";
    }

    // OK
    return true;
}

/// Wrap a hooked class
static bool WrapClass(const GeneratorInfo &info, WrapperImplState &state, const std::string &key, const nlohmann::json &obj) {
    // Get the outer revision
    std::string outerRevision = GetOuterRevision(info, key);

    // Common
    auto&& hooks = obj["hooks"];
    auto &&interfaces = info.specification["interfaces"];

    // Get vtable
    auto &&objInterface = interfaces[outerRevision];
    auto &&objVtbl = objInterface["vtable"];

    // Object common
    auto name = obj["name"].get<std::string>();

    // Requested key may differ
    std::string consumerKey = obj.contains("type") ? obj["type"].get<std::string>() : key;

    // Begin top images
    state.tbl << outerRevision << "DetourVTable " << key << "Wrapper::topImage = {\n";

    // Wrap the entire top image
    WrapClassTopImage(info, state, GetInterfaceBaseName(key), consumerKey, outerRevision, objInterface);

    // End top images
    state.tbl << "};\n\n";

    // Placeholder wrapper constructor
    state.ss << key << "Wrapper::" << key << "Wrapper() {\n";
    state.ss << "\t/* poof */\n";
    state.ss << "}\n";

    // Generate methods
    if (!WrapClassMethods(info, state, key, consumerKey, hooks, objInterface)) {
        return false;
    }

    // Generate wrapper detouring
    state.detours << consumerKey << "* CreateDetour(const Allocators& allocators, " << consumerKey << "* object, " << obj["state"].get<std::string>() << "* state) {\n";
    state.detours << "\tauto* wrapper = new (allocators) " << key << "Wrapper();\n";
    state.detours << "\twrapper->next = static_cast<" << outerRevision << "*>(object);\n";
    state.detours << "\twrapper->state = state;\n";
    state.detours << "\treturn reinterpret_cast<" << consumerKey << "*>(wrapper);\n";
    state.detours << "}\n\n";

    // Generate table getter
    state.getters << name << "Table GetTable(" << consumerKey << "* object) {\n";
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
