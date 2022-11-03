#include "GenTypes.h"
#include "Types.h"

// Std
#include <iostream>

struct ProxiesState {
    /// All streams
    std::stringstream proxies;
};

/// Wrap a hooked class
static void WrapClass(const GeneratorInfo &info, ProxiesState &state, const std::string &key, const nlohmann::json &obj) {
    if (!obj.contains("proxies")) {
        return;
    }

    // Begin class
    state.proxies << "struct " << key << "FeatureProxies {\n";
    state.proxies << "\tCommandContext* context{nullptr};\n";

    // Create proxies
    for (const std::string& proxy : obj["proxies"]) {
        // Generate feature bit set
        state.proxies << "\n\tuint64_t featureBitSet_" << proxy << "{0};\n";

        // Generate feature bit set mask
        state.proxies << "\tuint64_t featureBitSetMask_" << proxy << "{0};\n";

        // Generate feature callbacks
        state.proxies << "\tFeatureHook_" << proxy << "::Hook featureHooks_" << proxy << "[64];\n";
    }

    // End class
    state.proxies << "};\n\n";
}

bool Generators::FeatureProxies(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    ProxiesState state;

    // Common
    auto &&objects = info.hooks["objects"];

    // Wrap all hooked objects
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        WrapClass(info, state, it.key(), *it);
    }

    // Replace keys
    templateEngine.Substitute("$PROXIES", state.proxies.str().c_str());

    // OK
    return true;
}
