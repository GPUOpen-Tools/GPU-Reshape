using System;

namespace Studio.Plugin
{
    [Flags]
    public enum PluginResolveFlag
    {
        /// <summary>
        /// Continue plugin installation on failure
        /// </summary>
        ContinueOnFailure = 1
    }
}