using System.Reflection;

namespace Studio.Plugin
{
    class PluginState
    {
        /// <summary>
        /// Underlying assembly
        /// </summary>
        public Assembly? Assembly;

        /// <summary>
        /// Current mode
        /// </summary>
        public PluginMode Mode;

        /// <summary>
        /// Plugin handler
        /// </summary>
        public IPlugin? Plugin;
    }
}