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

    // Common
    auto &&interfaces = info.specification["interfaces"];

    // Get vtable
    auto &&objInterface = interfaces[outerRevision];

    // Object common
    auto objName = obj["name"].get<std::string>();
    auto objState = obj["state"].get<std::string>();

    // Forward declaration
    state.fwd << "struct " << objState << ";\n";

    // Create table
    state.tables << "struct " << objName << "Table {\n";
    state.tables << "\t" << outerRevision << "DetourVTable *bottom;\n";
    state.tables << "\t" << obj["state"].get<std::string>() << "* state;\n";
    state.tables << "\t" << outerRevision << "* next;\n";
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
