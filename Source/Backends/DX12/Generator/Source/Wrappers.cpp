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
