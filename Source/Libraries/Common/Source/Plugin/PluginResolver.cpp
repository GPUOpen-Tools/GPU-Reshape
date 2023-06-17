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

#include <Common/Plugin/PluginResolver.h>
#include <Common/Plugin/Plugin.h>
#include <Common/FileSystem.h>

// Xml
#include <tinyxml2.h>

// Std
#include <iostream>

PluginResolver::PluginResolver() {
    pluginPath = GetBaseModuleDirectory() / "Plugins";
}

bool PluginResolver::FindPlugins(const std::string_view &category, PluginList* list, PluginResolveFlagSet flags) {
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
                // Attempt to pre-load
                if (!FindPluginAtNode(catChildNode, list)) {
                    if (flags & PluginResolveFlag::ContinueOnFailure) {
                        continue;
                    }

                    // Considered failure
                    return false;
                }
            }
        }
    }

    // OK
    return true;
}

bool PluginResolver::FindPluginAtNode(tinyxml2::XMLNode *catChildNode, PluginList* list) {
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

    // Invoke info delegate
    delegate(&state.info);

    // Add entry
    PluginEntry pluginEntry{};
    pluginEntry.info = state.info;
    pluginEntry.plugin = name;
    list->plugins.push_back(pluginEntry);

    // OK
    return true;
}

bool PluginResolver::InstallPlugins(const PluginList &list, PluginResolveFlagSet flags) {
    std::vector<PluginEntry> entries = list.plugins;

    while (!entries.empty()) {
        bool mutated = false;

        for (int32_t i = static_cast<int32_t>(entries.size() - 1); i >= 0; i--) {
            PluginEntry& entry = entries[i];

            // Passed?
            bool pass{true};
            bool failedDependency{false};

            // Check all dependencies
            for (const std::string& dependency : entry.info.dependencies) {
                auto dep = plugins.find(dependency);
                if (dep == plugins.end()) {
                    std::cerr << "Plugin '" << entry.plugin << "' listed missing dependency '" << dependency << "'" << std::endl;
                    failedDependency = true;
                    break;
                }

                // Installed yet?
                if (dep->second.mode != PluginMode::Installed) {
                    pass = false;
                    break;
                }
            }

            // No dependency?
            if (failedDependency) {
                if (flags & PluginResolveFlag::ContinueOnFailure) {
                    continue;
                }
                return false;
            }

            // Not satisfied
            if (!pass) {
                continue;
            }

            // Attempt to install the plugin
            if (!InstallPlugin(entry)) {
                if (flags & PluginResolveFlag::ContinueOnFailure) {
                    continue;
                }
                return false;
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
        state.mode = PluginMode::None;
        return state;
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

bool PluginResolver::InstallPlugin(const PluginEntry &entry) {
    // Get the plugin state
    PluginState &state = GetPluginOrLoad(entry.plugin);

    // Check the plugin is in a good state
    if (state.mode == PluginMode::None) {
        return false;
    }

    // Already installed?
    if (state.mode == PluginMode::Installed) {
        return true;
    }

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

    // Mark as installed
    state.mode = PluginMode::Installed;

    // OK
    return true;
}
