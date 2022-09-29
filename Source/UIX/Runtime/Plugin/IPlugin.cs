namespace Studio.Plugin
{
    public interface IPlugin
    {
        /// <summary>
        /// General plugin info
        /// </summary>
        PluginInfo Info { get; }
        
        /// <summary>
        /// Install this plugin
        /// </summary>
        /// <returns>false if failed</returns>
        bool Install();

        /// <summary>
        /// Uninstall this plugin
        /// </summary>
        void Uninstall();
    }
}