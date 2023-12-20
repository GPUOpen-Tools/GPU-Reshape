// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
