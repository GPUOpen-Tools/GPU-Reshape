using System;
using Studio.ViewModels.Workspace;

namespace Studio.ViewModels.Documents
{
    public class WorkspaceOverviewDescriptor : IDescriptor
    {
        /// <summary>
        /// Sortable identifier
        /// </summary>
        public object? Identifier => Tuple.Create(typeof(WorkspaceOverviewDescriptor), Workspace);

        /// <summary>
        /// Workspace view mode
        /// </summary>
        public IWorkspaceViewModel? Workspace { get; set; }
    }
}