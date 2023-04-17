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
        WorkspaceTool = 1,
        
        /// <summary>
        /// Summary view in the workspace overview
        /// </summary>
        WorkspaceOverview = 2,
        
        /// <summary>
        /// Property tool data section
        /// </summary>
        Configuration = 4,
    }
}