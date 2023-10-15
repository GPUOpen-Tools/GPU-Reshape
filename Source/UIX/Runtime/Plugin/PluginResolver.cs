// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.Loader;
using System.Xml;

namespace Studio.Plugin
{
    public class PluginResolver
    {
        /// <summary>
        /// Find all plugins with a given category
        /// </summary>
        /// <param name="category"></param>
        /// <param name="flags"></param>
        /// <returns></returns>
        public PluginList? FindPlugins(string category, PluginResolveFlag flags)
        {
            List<PluginEntry> entries = new();

            foreach (string file in Directory.GetFiles("Plugins", "*.xml"))
            {
                // Load xml from path
                XmlDocument doc = new();
                doc.Load(file);

                // Get root specification
                XmlNode? spec = doc.GetElementsByTagName("spec").Item(0);
                if (spec == null)
                {
                    // May continue?
                    if (flags.HasFlag(PluginResolveFlag.ContinueOnFailure))
                        continue;

                    return null;
                }

                // Get all category entries
                foreach (XmlElement entry in spec.ChildNodes)
                {
                    // Filter by category
                    if (entry.Name != category)
                        continue;
                    
                    // Each entry may contain a list of plugins
                    foreach (XmlElement plugin in entry.ChildNodes)
                    {
                        PluginEntry? pluginEntry = FindPluginAtElement(plugin);
                        if (pluginEntry == null)
                        {
                            // May continue?
                            if (flags.HasFlag(PluginResolveFlag.ContinueOnFailure))
                                continue;

                            return null;
                        }

                        entries.Add(pluginEntry.Value);
                    }
                }
            }

            // Create list
            return new PluginList()
            {
                Entries = entries.ToArray()
            };
        }

        /// <summary>
        /// Find a plugin from an element
        /// </summary>
        /// <param name="plugin"></param>
        /// <returns></returns>
        private PluginEntry? FindPluginAtElement(XmlElement plugin)
        {
            // Validate tag
            if (plugin.Name != "plugin")
                return null;

            // Get name
            string name = plugin.GetAttribute("name");
            if (name == string.Empty)
                return null;

            // Try to get plugin
            PluginState? state = GetPluginOrLoad(name);
            if (state == null)
                return null;

            // Create entry
            PluginEntry entry;
            entry.Info = state.Plugin!.Info;
            entry.Name = name;
            return entry;
        }

        /// <summary>
        /// Get a loaded plugin, or load the assemblies
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        private PluginState? GetPluginOrLoad(string name)
        {
            if (_states.ContainsKey(name))
                return _states[name];

            // Create state
            var state = new PluginState()
            {
                Mode = PluginMode.Loaded
            };

            string path = name + ".dll";

            // Must exist
            if (!System.IO.File.Exists(path))
                return null;

            // Attempt to load
            try
            {
                state.Assembly = Assembly.LoadFrom(path);
            }
            catch
            {
                state.Assembly = null;
            }
            
            // Failed to load?
            if (state.Assembly == null)
                return null;

            // Enumerate and instantiate plugins
            foreach (Type type in state.Assembly.GetTypes())
            {
                // Is plugin?
                if (!typeof(IPlugin).IsAssignableFrom(type))
                    continue;

                // Can instantiate?
                if (Activator.CreateInstance(type) is not IPlugin result)
                    continue;

                state.Plugin = result;
                break;
            }

            // No plugin?
            if (state.Plugin == null)
                return null;

            _states.Add(name, state);
            return state;
        }

        /// <summary>
        /// Install all plugins from a list
        /// </summary>
        /// <param name="list"></param>
        /// <param name="flags"></param>
        /// <returns></returns>
        public bool InstallPlugins(PluginList list, PluginResolveFlag flags)
        {
            List<PluginEntry> entries = list.Entries.ToList();

            while (entries.Count > 0)
            {
                bool mutated = false;

                for (int i = entries.Count - 1; i >= 0; i--)
                {
                    bool pass = true;

                    // Validate dependencies
                    foreach (string dependency in entries[i].Info.Dependencies)
                    {
                        // Try to find dependency
                        PluginState? dependencyState = _states.GetValueOrDefault(dependency);
                        if (dependencyState?.Mode == PluginMode.Installed)
                            continue;

                        // Mark as failed
                        pass = false;
                        break;
                    }

                    // Did a dependency fail?
                    if (!pass)
                        continue;

                    // Failed to install?
                    if (!InstallPlugin(entries[i]) && !flags.HasFlag(PluginResolveFlag.ContinueOnFailure))
                    {
                        return false;
                    }

                    // Remove
                    entries.RemoveAt(i);
                    mutated = true;
                }

                // Resolved nothing?
                if (!mutated)
                    break;
            }

            // Validate all resolved
            if (entries.Count > 0 && !flags.HasFlag(PluginResolveFlag.ContinueOnFailure))
            {
                return false;
            }

            // OK
            return true;
        }

        /// <summary>
        /// Install a plugin
        /// </summary>
        /// <param name="entry"></param>
        /// <returns></returns>
        private bool InstallPlugin(PluginEntry entry)
        {
            // Get plugin state
            PluginState? state = _states.GetValueOrDefault(entry.Name);
            if (state == null || state.Mode == PluginMode.None)
                return false;

            // Already installed?
            if (state.Mode == PluginMode.Installed)
                return true;

            // Attempt to install
            if (!state.Plugin!.Install())
                return false;

            // OK
            state.Mode = PluginMode.Installed;
            return true;
        }

        /// <summary>
        /// Uninstall all loaded plugins
        /// </summary>
        public void Uninstall()
        {
            foreach (var kv in _states)
            {
                if (kv.Value.Mode != PluginMode.Installed)
                    continue;

                // Uninstall state
                kv.Value.Plugin!.Uninstall();
                kv.Value.Mode = PluginMode.Loaded;
            }
        }

        /// <summary>
        /// Currently loaded plugins
        /// </summary>
        private Dictionary<string, PluginState> _states = new();
    }
}