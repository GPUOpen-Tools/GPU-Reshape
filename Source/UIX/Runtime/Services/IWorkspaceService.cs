using System;
using System.Reactive.Subjects;
using DynamicData;
using ReactiveUI;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.Services
{
    public interface IWorkspaceService : IReactiveObject
    {
        /// <summary>
        /// All active workspaces
        /// </summary>
        IObservableList<ViewModels.Workspace.IWorkspaceViewModel> Workspaces { get; }
        
        /// <summary>
        /// Current workspace
        /// </summary>
        public ViewModels.Workspace.IWorkspaceViewModel? SelectedWorkspace { get; set; }
        
        /// <summary>
        /// Current selected property
        /// </summary>
        public IPropertyViewModel? SelectedProperty { get; set; }
        
        /// <summary>
        /// Add a new workspace
        /// </summary>
        /// <param name="workspaceViewModel">active connection</param>
        void Add(ViewModels.Workspace.IWorkspaceViewModel workspaceViewModel);

        /// <summary>
        /// Remove an existing workspace
        /// </summary>
        /// <param name="workspaceViewModel">active connection</param>
        /// <returns>true if successfully removed</returns>
        bool Remove(ViewModels.Workspace.IWorkspaceViewModel workspaceViewModel);

        /// <summary>
        /// Clear all workspaces
        /// </summary>
        void Clear();
    }
}