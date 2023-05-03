#include "GenTypes.h"
#include "Types.h"

// Std
#include <vector>
#include <iostream>

struct ObjectWrappersState {
    /// All streams
    std::stringstream includes;
    std::stringstream hooks;
    std::stringstream detours;
    std::stringstream getters;
};

/// Wrap the base interface queries
static void WrapClassBaseQuery(const GeneratorInfo &info, ObjectWrappersState& state, const std::string& key, bool top = true) {
    auto&& obj = info.specification["interfaces"][key];

    // Keep it clean
    if (!top) {
        state.hooks << " else ";
    }

    // Query check
    state.hooks << "if (riid == __uuidof(" << key << ")) {\n";
    state.hooks << "\t\t\tAddRef();\n";
    state.hooks << "\t\t\t*ppvObject = static_cast<" << key << "*>(this);\n";
    state.hooks << "\t\t\treturn S_OK;\n";
    state.hooks << "\t\t}";

    // Query all bases
    for (auto&& base : obj["bases"]) {
        WrapClassBaseQuery(info, state, base.get<std::string>(), false);
    }
}

/// Wrap all class methods
static bool WrapClassMethods(const GeneratorInfo &info, ObjectWrappersState &state, const std::string &key, const std::string& consumerKey, const nlohmann::json &objDecl, const nlohmann::json &obj) {
    // For all bases
    for (auto&& base : obj["bases"]) {
        auto&& baseInterface = info.specification["interfaces"][base.get<std::string>()];

        // Wrap base image
        WrapClassMethods(info, state, key, consumerKey, objDecl, baseInterface);
    }

    // Get the outer revision
    std::string outerRevision = GetOuterRevision(info, key);

    // Generate wrappers
    for (auto &&method: obj["vtable"]) {
        // Common
        auto methodName = method["name"].get<std::string>();

        // Get parameters
        auto &&parameters = method["params"];

        // Qualifiers
        state.hooks << "\tvirtual ";

        // Print return type
        if (!PrettyPrintType(state.hooks, method["returnType"])) {
            return false;
        }

        // Print name of wrapper
        state.hooks << " " << methodName << "(";

        for (size_t i = 0; i < parameters.size(); i++) {
            auto&& param = parameters[i];

            // Separator
            if (i > 0) {
                state.hooks << ", ";
            }

            // Dedicated printer
            if (!PrettyPrintParameter(state.hooks, param["type"], param["name"].get<std::string>())) {
                return false;
            }
        }

        // Begin body
        state.hooks << ") override {\n";

        // Check if this wrapper is hooked
        bool isHooked{false};
        for (auto hook: objDecl["hooks"]) {
            isHooked |= hook.get<std::string>() == methodName;
        }

        // Structural return type? (kept in for compatability with other wrappers)
        bool isStructRet = isHooked && IsTypeStruct(method["returnType"]);

        // If structural, declare local value
        if (isStructRet) {
            state.hooks << "\t\t";

            // Print return type
            if (!PrettyPrintType(state.hooks, method["returnType"])) {
                return false;
            }

            state.hooks << " out;\n";
        }

        // Proxied?
        if (objDecl.contains("proxies")) {
            bool isProxied{false};

            // Check if it exists in the shared list
            for (auto proxy : objDecl["proxies"]) {
                isProxied |= proxy.get<std::string>() == methodName;
            }

            // Proxied hook?
            if (isProxied) {
                // Pass down to next object
                state.hooks << "\t\tif (ApplyFeatureHook<FeatureHook_" << methodName << ">(\n";
                state.hooks << "\t\t\tstate,\n";
                state.hooks << "\t\t\tstate->proxies.context,\n";
                state.hooks << "\t\t\tstate->proxies.featureBitSet_" << methodName << ",\n";
                state.hooks << "\t\t\tstate->proxies.featureHooks_" << methodName;

                // Any parameters?
                if (parameters.size()) {
                    state.hooks << ",\n";
                    state.hooks << "\t\t\t";

                    // Unwrap arguments
                    for (size_t i = 0; i < parameters.size(); i++) {
                        if (i != 0) {
                            state.hooks << ", ";
                        }

                        state.hooks << parameters[i]["name"].get<std::string>();
                    }
                } else {
                    state.hooks << "\n";
                }

                // End call
                state.hooks << "\n\t\t)) {\n";
                state.hooks << "\t\t\tCommitCommands(state);\n";
                state.hooks << "\t\t}\n\n";
            }
        }
        
        // Hooked?
        if (isHooked) {
            // Print return if needed
            if (isStructRet || method["returnType"]["type"] == "void") {
                state.hooks << "\t";
            } else {
                state.hooks << "\t\treturn ";
            }

            // Proxy down to hook
            state.hooks << "\tHook" << consumerKey << methodName << "(this";

            // Forward parameters
            for (size_t i = 0; i < parameters.size(); i++) {
                state.hooks << ", " << parameters[i]["name"].get<std::string>();
            }

            // If structural, append the output value
            if (isStructRet) {
                state.hooks << ", ";
                state.hooks << "&out";
            }

            // End call
            state.hooks << ");\n";

            // If structural, return the value!
            if (isStructRet) {
                state.hooks << "\t\treturn out;\n";
            }

            // End function
            state.hooks << "\t}\n\n";
        } else {
            // Interface querying
            if (methodName == "QueryInterface") {
                // Check state query
                state.hooks << "\t\tif (riid == __uuidof(" << objDecl["state"].get<std::string>() << ")) {\n";
                state.hooks << "\t\t\t/* No ref added */\n";
                state.hooks << "\t\t\t*ppvObject = state;\n";
                state.hooks << "\t\t\treturn S_OK;\n";
                state.hooks << "\t\t}\n\n";
                
                // If unwrapping, simply proxy down the next object
                state.hooks << "\t\tif (riid == IID_Unwrap) {\n";
                state.hooks << "\t\t\t/* No ref added */\n";
                state.hooks << "\t\t\t*ppvObject = next;\n";
                state.hooks << "\t\t\treturn S_OK;\n";
                state.hooks << "\t\t}\n\n";

                // Wrap query
                state.hooks << "\t\t";
                WrapClassBaseQuery(info, state, outerRevision);

                // Unknown interface, pass down to next object
                // TODO: This is not safe at all, ignores reference counting mechanics!
                state.hooks << "\n\n\t\treturn next->" << methodName << "(riid, ppvObject);\n";
                state.hooks << "\t}\n\n";
            }

            // Add a refernece to this object
            else if (methodName == "AddRef") {
                state.hooks << "\t\treturn static_cast<ULONG>(++users);\n";
                state.hooks << "\t}\n\n";
            }

            // Release a reference to this object
            else if (methodName == "Release") {
                state.hooks << "\t\tint64_t references = --users;\n";
                state.hooks << "\t\tif (references == 0) {\n";

                state.hooks << "\t\t\tif constexpr(IsReferenceObject<decltype(state)>) {\n";
                state.hooks << "\t\t\tdestroyRef(state, state->allocators);\n\n";
                state.hooks << "\t\t\t} else {\n";
                state.hooks << "\t\t\tdestroy(state, state->allocators);\n\n";
                state.hooks << "\t\t\t}\n";

                state.hooks << "\t\t\t/* Release the bottom reference */\n";
                state.hooks << "\t\t\tULONG bottom = next->Release();\n";
                state.hooks << "\t\t\t(void)bottom;\n";

                state.hooks << "\t\t}\n\n";
                state.hooks << "\t\treturn static_cast<ULONG>(references);\n";
                state.hooks << "\t}\n\n";
            }

            // Regular hook
            else {
                // Print return if needed
                if (method["returnType"]["type"] == "void") {
                    state.hooks << "\t\t";
                } else {
                    state.hooks << "\t\treturn ";
                }

                // Pass down to next object
                state.hooks << "next->" << methodName << "(";

                // Unwrap arguments
                for (size_t i = 0; i < parameters.size(); i++) {
                    if (i != 0) {
                        state.hooks << ", ";
                    }

                    state.hooks << "Unwrap(" << parameters[i]["name"].get<std::string>() << ")";
                }

                // End call
                state.hooks << ");\n";

                // End function
                state.hooks << "\t}\n\n";
            }
        }
    }

    // OK
    return true;
}

/// Wrap a hooked class
static bool WrapClass(const GeneratorInfo &info, ObjectWrappersState &state, const std::string &key, const nlohmann::json &obj) {
    // Get the outer revision
    std::string outerRevision = GetOuterRevision(info, key);

    // Common
    auto &&interfaces = info.specification["interfaces"];

    // Get vtable
    auto &&objInterface = interfaces[outerRevision];

    // Object common
    auto name = obj["name"].get<std::string>();

    // Requested key may differ
    std::string consumerKey = obj.contains("type") ? obj["type"].get<std::string>() : key;

    // Begin class
    state.hooks << "class " << key << "Wrapper final : public " << outerRevision << " {\n";
    state.hooks << "public:\n";

    // Member chain
    state.hooks << "\t/* Next object on this chain */\n";
    state.hooks << "\t" << outerRevision << "* next;\n\n";

    // State
    state.hooks << "\t/* Internal state of this object */\n";
    state.hooks << "\t" << obj["state"].get<std::string>() << "* state;\n\n";

    // User counter
    state.hooks << "\t/* Internal user count */\n";
    state.hooks << "\tstd::atomic<int64_t> users{1};\n\n";

    // Generate methods
    if (!WrapClassMethods(info, state, key, consumerKey, obj, objInterface)) {
        return false;
    }

    // End class
    state.hooks << "};\n\n";

    // Generate wrapper detouring
    state.detours << consumerKey << "* CreateDetour(const Allocators& allocators, " << consumerKey << "* object, " << obj["state"].get<std::string>() << "* state) {\n";
    state.detours << "\tauto* wrapper = new (allocators) " << key << "Wrapper();\n";
    state.detours << "\twrapper->next = static_cast<" << outerRevision << "*>(object);\n";
    state.detours << "\twrapper->state = state;\n";
    state.detours << "\treturn static_cast<" << consumerKey << "*>(wrapper);\n";
    state.detours << "}\n\n";

    // Generate table getter
    state.getters << name << "Table GetTable(" << consumerKey << "* object) {\n";
    state.getters << "\tauto wrapper = static_cast<" << key << "Wrapper*>(object);\n";
    state.getters << "\tif (!wrapper) {\n";
    state.getters << "\t\treturn {};\n";
    state.getters << "\t}\n\n";
    state.getters << "\t" << name << "Table table;\n";
    state.getters << "\ttable.next = wrapper->next;\n";
    state.getters << "\ttable.bottom = GetVTableRaw<" << key << "TopDetourVTable>(wrapper->next);\n";
    state.getters << "\ttable.state = wrapper->state;\n";
    state.getters << "\treturn table;\n";
    state.getters << "}\n\n";

    // OK
    return true;
}

bool Generators::ObjectWrappers(const GeneratorInfo &info, TemplateEngine &templateEngine) {
    ObjectWrappersState state;

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
    templateEngine.Substitute("$HOOKS", state.hooks.str().c_str());
    templateEngine.Substitute("$DETOURS", state.detours.str().c_str());
    templateEngine.Substitute("$GETTERS", state.getters.str().c_str());

    // OK
    return true;
}
