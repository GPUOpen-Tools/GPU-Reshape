using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Reactive.Linq;
using DynamicData;
using DynamicData.Binding;

namespace Studio.Services
{
    public class WorkspaceService : IWorkspaceService
    {
        /// <summary>
        /// Active workspaces
        /// </summary>
        public IObservableList<ViewModels.Workspace.IWorkspaceViewModel> Workspaces => _workspaces;

        public WorkspaceService()
        {
            
        }
        
        /// <summary>
        /// Add a new workspace
        /// </summary>
        /// <param name="workspaceViewModel">active connection</param>
        public void Add(ViewModels.Workspace.IWorkspaceViewModel workspaceViewModel)
        {
            _workspaces.Add(workspaceViewModel);
            
            // Diagnostic
            Logging.Info($"Workspace created for {workspaceViewModel.Connection?.Application?.Name} {{{workspaceViewModel.Connection?.Application?.Guid}}}");
        }

        /// <summary>
        /// Remove an existing workspace
        /// </summary>
        /// <param name="workspaceViewModel">active connection</param>
        /// <returns>true if successfully removed</returns>
        public bool Remove(ViewModels.Workspace.IWorkspaceViewModel workspaceViewModel)
        {
            // Diagnostic
            Logging.Info($"Closed workspace for {workspaceViewModel.Connection?.Application?.Name}");
            
            // Try to remove
            return _workspaces.Remove(workspaceViewModel);
        }

        /// <summary>
        /// Internal active workspaces
        /// </summary>
        private SourceList<ViewModels.Workspace.IWorkspaceViewModel> _workspaces = new();
        
    }
}