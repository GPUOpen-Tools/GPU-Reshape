namespace Studio.Plugin
{
    enum PluginMode
    {
        /// <summary>
        /// Plugin not loaded
        /// </summary>
        None,
        
        /// <summary>
        /// Plugin assemblies are in memory
        /// </summary>
        Loaded,
        
        /// <summary>
        /// Plugin is currently installed
        /// </summary>
        Installed
    }
}