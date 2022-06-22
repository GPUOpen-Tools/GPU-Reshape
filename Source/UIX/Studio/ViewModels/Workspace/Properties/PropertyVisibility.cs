using System;

namespace Studio.ViewModels.Workspace.Properties
{
    [Flags]
    public enum PropertyVisibility
    {
        /// <summary>
        /// Standard visibility
        /// </summary>
        Default = 0,
        
        /// <summary>
        /// Minimal view on workspace explorer tool
        /// </summary>
        WorkspaceTool = 1
    }
}