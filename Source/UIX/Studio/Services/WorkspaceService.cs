using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Reactive.Linq;
using Avalonia;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Workspace;

namespace Studio.Services
{
    public class WorkspaceService : ReactiveObject, IWorkspaceService
    {
        /// <summary>
        /// Active workspaces
        /// </summary>
        public IObservableList<ViewModels.Workspace.IWorkspaceViewModel> Workspaces => _workspaces;

        /// <summary>
        /// Current workspace
        /// </summary>
        public IWorkspaceViewModel? SelectedWorkspace
        {
            get => _selectedWorkspace;
            set => this.RaiseAndSetIfChanged(ref _selectedWorkspace, value);
        }

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
            
            // Submit document
            if (App.Locator.GetService<IWindowService>()?.LayoutViewModel is { } layoutViewModel)
            {
                layoutViewModel.OpenDocument?.Execute(new WorkspaceOverviewDescriptor()
                {
                    Workspace = workspaceViewModel 
                });
            }
            
            // Diagnostic
            Logging.Info($"Workspace created for {workspaceViewModel.Connection?.Application?.Name} {{{workspaceViewModel.Connection?.Application?.Guid}}}");
            
            // Assign as selected
            SelectedWorkspace = workspaceViewModel;
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
            
            // Is selected?
            if (SelectedWorkspace == workspaceViewModel)
            {
                SelectedWorkspace = null;
            }
            
            // Try to remove
            return _workspaces.Remove(workspaceViewModel);
        }

        /// <summary>
        /// Clear all workspaces
        /// </summary>
        public void Clear()
        {
            if (_workspaces.Count > 0)
            {
                // Diagnostic
                Logging.Info($"Closing all workspaces");

                // Remove selected
                SelectedWorkspace = null;
            
                // Remove all
                _workspaces.Clear();
            }
        }

        /// <summary>
        /// Internal active workspaces
        /// </summary>
        private SourceList<ViewModels.Workspace.IWorkspaceViewModel> _workspaces = new();

        /// <summary>
        /// Internal active workspace
        /// </summary>
        private IWorkspaceViewModel? _selectedWorkspace;

    }
}