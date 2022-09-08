#pragma once

// Common
#include <Common/Library.h>
#include <Common/IComponent.h>
#include <Common/Plugin/Plugin.h>
#include <Common/Plugin/PluginInfo.h>
#include <Common/Plugin/PluginList.h>
#include <Common/Plugin/PluginResolveFlag.h>

// Std
#include <string_view>
#include <filesystem>
#include <string>
#include <map>

// Forward declarations
namespace tinyxml2 {
    class XMLNode;
}

class PluginResolver : public TComponent<PluginResolver> {
public:
    COMPONENT(PluginResolver);

    PluginResolver();

    /// Find all plugins of a certain category
    /// \param category the category to be found
    /// \param list the output list
    /// \param flags the resolve flags
    /// \return true if successful
    bool FindPlugins(const std::string_view& category, PluginList* list, PluginResolveFlagSet flags = PluginResolveFlag::None);

    /// Install all plugins from a list
    /// \param list the list of plugins to be installed
    /// \param flags the resolve flags
    /// \return true if successful
    bool InstallPlugins(const PluginList& list, PluginResolveFlagSet flags = PluginResolveFlag::None);

    /// Uninstall all installed plugins
    void Uninstall();

private:
    bool FindPluginAtNode(tinyxml2::XMLNode* catChildNode, PluginList* list);

    bool InstallPlugin(const PluginEntry& entry);

private:
    /// Internal plugin mode
    enum class PluginMode {
        None,
        Loaded,
        Installed
    };

    /// State of a plugin
    struct PluginState {
        Library library;
        PluginInfo info;
        PluginMode mode{PluginMode::None};
    };

    /// Get a plugin, load if not already loaded
    /// \param path path of the plugin
    /// \return the state
    PluginState& GetPluginOrLoad(const std::string& path);

private:
    /// Plugins path
    std::filesystem::path pluginPath;

    /// All loaded plugins
    std::map<std::string, PluginState> plugins;
};
