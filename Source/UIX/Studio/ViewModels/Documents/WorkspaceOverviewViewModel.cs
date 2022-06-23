using Dock.Model.ReactiveUI.Controls;

namespace Studio.ViewModels.Documents
{
    public class WorkspaceOverviewViewModel : Document
    {
        /// <summary>
        /// Workspace within this overview
        /// </summary>
        public Workspace.WorkspaceViewModel? Workspace { get; set; }
    }
}