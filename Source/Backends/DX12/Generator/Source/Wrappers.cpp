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

/// Wrap a hooked object
static bool WrapClass(const GeneratorInfo &info, WrapperState& state, const std::string& key, const nlohmann::json& obj) {
    // Get the outer revision
    std::string outerRevision = GetOuterRevision(info, key);

    // Common
    auto&& interfaces = info.specification["interfaces"];

    // Get the vtable
    auto&& objInterface = interfaces[outerRevision];
    auto&& objVtbl = interfaces[std::string(objInterface["vtable"])];

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

    // Generate hooks prototypes
    for (auto&& field : objVtbl["fields"]) {
        auto &&type = field["type"];

        // Common
        auto fieldName = field["name"].get<std::string>();

        // Skip non function pointers
        if (type["type"] != "pointer" && type["contained"]["type"] != "function") {
            continue;
        }

        // Get contained (fptr) type
        auto &&funcType   = type["contained"];
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

        // End wrapper
        state.hooks << ");\n";
    }

    // OK
    return true;
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
