namespace Studio.Plugin
{
    public struct PluginInfo
    {
        /// <summary>
        /// Name of this plugin
        /// </summary>
        public string Name;

        /// <summary>
        /// Brief description of this plugin
        /// </summary>
        /// <returns></returns>
        public string Description;

        /// <summary>
        /// All plugin dependencies
        /// </summary>
        public string[] Dependencies;
    }
}