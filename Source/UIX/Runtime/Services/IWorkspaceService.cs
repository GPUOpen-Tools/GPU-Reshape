using System;
using System.Reactive.Subjects;
using DynamicData;
using ReactiveUI;
using Runtime.ViewModels.Shader;
using Studio.ViewModels.Traits;
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
        /// All extensions
        /// </summary>
        public ISourceList<IWorkspaceExtension> Extensions { get; }
        
        /// <summary>
        /// Current workspace
        /// </summary>
        public ViewModels.Workspace.IWorkspaceViewModel? SelectedWorkspace { get; set; }
        
        /// <summary>
        /// Current selected property
        /// </summary>
        public IPropertyViewModel? SelectedProperty { get; set; }
        
        /// <summary>
        /// Current selected property
        /// </summary>
        public ShaderNavigationViewModel? SelectedShader { get; set; }
        
        /// <summary>
        /// Add a new workspace
        /// </summary>
        /// <param name="workspaceViewModel">active connection</param>
        void Add(ViewModels.Workspace.IWorkspaceViewModel workspaceViewModel);

        /// <summary>
        /// Install a given workspace
        /// </summary>
        /// <param name="workspaceViewModel"></param>
        void Install(ViewModels.Workspace.IWorkspaceViewModel workspaceViewModel);

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