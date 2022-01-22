#include <Common/Plugin/PluginResolver.h>
#include <Common/Plugin/Plugin.h>

// Xml
#include <tinyxml2.h>

// Std
#include <filesystem>

// Std
#include <iostream>

PluginResolver::PluginResolver() {
    pluginPath = std::filesystem::current_path() / "Plugins";
}

bool PluginResolver::FindPlugins(const std::string_view &category, PluginList* list) {
    for (const auto & entry : std::filesystem::directory_iterator(pluginPath)) {
        // TODO: Casing?
        if (entry.path().extension() != ".xml") {
            continue;
        }

        // Attempt to open the xml
        tinyxml2::XMLDocument document;
        if (document.LoadFile(entry.path().string().c_str()) != tinyxml2::XML_SUCCESS) {
            std::cerr << "Failed to parse plugin xml: " << entry.path() << std::endl;
            return false;
        }

        // Get the root spec
        tinyxml2::XMLElement *specNode = document.FirstChildElement("spec");
        if (!specNode) {
            std::cerr << "Failed to find root spec in xml" << std::endl;
            return false;
        }

        // Process all nodes
        for (tinyxml2::XMLNode *catNode = specNode->FirstChild(); catNode; catNode = catNode->NextSibling()) {
            tinyxml2::XMLElement *cat = catNode->ToElement();
            if (!cat) {
                continue;
            }

            // Not a match?
            if (category != cat->Name()) {
                continue;
            }

            // Process all plugins
            for (tinyxml2::XMLNode *catChildNode = cat->FirstChild(); catChildNode; catChildNode = catChildNode->NextSibling()) {
                tinyxml2::XMLElement *catChild = catChildNode->ToElement();

                // Must be plugin under categories
                if (std::strcmp(catChild->Name(), "plugin")) {
                    std::cerr << "Category child must be plugin in line: " << catChild->GetLineNum() << std::endl;
                    return false;
                }

                // Get the name
                const char *name = catChild->Attribute("name", nullptr);
                if (!name) {
                    std::cerr << "Malformed command in line: " << catChild->GetLineNum() << ", name not found" << std::endl;
                    return false;
                }

                // Attempt to load
                PluginState &state = GetPluginOrLoad(name);
                if (!state.library.IsGood()) {
                    return false;
                }

                // Get the info delegate
                auto* delegate = state.library.GetFn<PluginInfoDelegate>(PLUGIN_INFO_S);
                if (!delegate) {
                    std::cerr << "Plugin '" << name << "' missing entrypoint '" << PLUGIN_INFO_S << "'" << std::endl;
                    return false;
                }

                delegate(&state.info);

                PluginEntry pluginEntry{};
                pluginEntry.info = state.info;
                pluginEntry.plugin = name;
                list->plugins.push_back(pluginEntry);
            }
        }
    }

    // OK
    return true;
}

bool PluginResolver::InstallPlugins(const PluginList &list) {
    std::vector<PluginEntry> entries = list.plugins;

    while (!entries.empty()) {
        bool mutated = false;

        for (int32_t i = static_cast<int32_t>(entries.size() - 1); i >= 0; i--) {
            PluginEntry& entry = entries[i];

            // Passed?
            bool pass{true};

            // Check all dependencies
            for (const std::string& dependency : entry.info.dependencies) {
                auto dep = plugins.find(dependency);
                if (dep == plugins.end()) {
                    std::cerr << "Plugin '" << entry.plugin << "' listed missing dependency '" << dependency << "'" << std::endl;
                    return false;
                }

                // Installed yet?
                if (dep->second.mode != PluginMode::Installed) {
                    pass = false;
                    break;
                }
            }

            // Not satisfied
            if (!pass) {
                continue;
            }

            PluginState &state = GetPluginOrLoad(entry.plugin);

            if (state.mode != PluginMode::Installed) {
                // Get the install delegate
                auto* delegate = state.library.GetFn<PluginInstallDelegate>(PLUGIN_INSTALL_S);
                if (!delegate) {
                    std::cerr << "Plugin '" << entry.plugin << "' missing entrypoint '" << PLUGIN_INSTALL_S << "'" << std::endl;
                    return false;
                }

                // Try to install
                if (!delegate(registry)) {
                    std::cerr << "Plugin '" << entry.plugin << "' failed to install" << std::endl;
                    return false;
                }
            }

            // Remove
            entries.erase(entries.begin() + i);

            // Mutated
            mutated = true;
        }

        // Failed to resolve?
        if (!mutated) {
            for (const PluginEntry& entry : entries) {
                std::cerr << "Plugin '" << entry.plugin << "' failed to load, plugin dependencies was not satisfied:" << std::endl;

                for (const std::string& dependency : entry.info.dependencies) {
                    std::cerr << "Dependency '" << dependency << "'" << std::endl;
                }
            }
            return false;
        }
    }

    return true;
}

PluginResolver::PluginState &PluginResolver::GetPluginOrLoad(const std::string &path) {
    auto it = plugins.find(path);
    if (it != plugins.end()) {
        return it->second;
    }

    PluginState& state = plugins[path];
    if (!state.library.Load((pluginPath / path).string().c_str())) {
        std::cerr << "Failed to load plugin '" << path << "'" << std::endl;
    }

    state.mode = PluginMode::Loaded;
    return state;
}

void PluginResolver::Uninstall() {
    for (;;) {
        bool mutated = false;

        for (auto&& kv : plugins) {
            PluginState& state = kv.second;

            // Installed?
            if (state.mode != PluginMode::Installed) {
                continue;
            }

            // Passed?
            bool pass{true};

            // Check all dependencies
            for (const std::string& dependency : state.info.dependencies) {
                auto dep = plugins.find(dependency);
                if (dep == plugins.end()) {
                    continue;
                }

                // Uninstalled yet?
                if (dep->second.mode == PluginMode::Installed) {
                    pass = false;
                    break;
                }
            }

            // Next!
            if (!pass) {
                continue;
            }

            // Get the uninstall delegate (optional)
            auto* delegate = state.library.GetFn<PluginUninstallDelegate>(PLUGIN_UNINSTALL_S);
            if (!delegate) {
                continue;
            }

            // Uninstall
            delegate(registry);

            // Mark as uninstalled
            state.mode = PluginMode::Loaded;

            // Mutated
            mutated = true;
        }

        if (!mutated) {
            break;
        }
    }
}
