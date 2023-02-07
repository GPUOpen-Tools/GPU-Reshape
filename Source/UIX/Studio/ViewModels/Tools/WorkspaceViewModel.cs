using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Windows.Input;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Dock.Model.ReactiveUI.Controls;
using DynamicData;
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Workspace;
using Studio.Views;

namespace Studio.ViewModels.Tools
{
    public class WorkspaceViewModel : Tool
    {
        /// <summary>
        /// Connect to new workspace
        /// </summary>
        public ICommand Connect { get; }
        
        /// <summary>
        /// Connect to new workspace
        /// </summary>
        public ICommand Refresh { get; }
        
        /// <summary>
        /// Connect to new workspace
        /// </summary>
        public ICommand Expand { get; }
        
        /// <summary>
        /// Connect to new workspace
        /// </summary>
        public ICommand Collapse { get; }
        
        /// <summary>
        /// Connect to new workspace
        /// </summary>
        public ICommand ClearAll { get; }

        /// <summary>
        /// All workspaces
        /// </summary>
        public ObservableCollection<Controls.IObservableTreeItem> Workspaces { get; } = new();

        /// <summary>
        /// Is the help message visible?
        /// </summary>
        public bool IsHelpVisible
        {
            get => _isHelpVisible;
            set => this.RaiseAndSetIfChanged(ref _isHelpVisible, value);
        }

        public WorkspaceViewModel()
        {
            Connect = ReactiveCommand.Create(OnConnect);
            Refresh = ReactiveCommand.Create(OnRefresh);
            Expand = ReactiveCommand.Create(OnExpand);
            Collapse = ReactiveCommand.Create(OnCollapse);
            ClearAll = ReactiveCommand.Create(OnClearAll);

            // Get service
            _workspaceService = App.Locator.GetService<Services.IWorkspaceService>() ?? throw new InvalidOperationException();

            // On workspaces updated
            _workspaceService.Workspaces.Connect()
                .OnItemAdded(x => OnWorkspaceAdded(x))
                .OnItemRemoved(x => OnWorkspaceRemoved(x))
                .Subscribe();
        }

        /// <summary>
        /// Invoked when a workspace has been added to the service
        /// </summary>
        /// <param name="connectionViewModel">added workspace</param>
        public void OnWorkspaceAdded(Workspace.IWorkspaceViewModel workspaceViewModel)
        {
            Workspaces.Add(new Controls.WorkspaceTreeItemViewModel()
            {
                OwningContext = workspaceViewModel,
                ViewModel = workspaceViewModel.PropertyCollection
            });

            IsHelpVisible = false;
        }

        /// <summary>
        /// Invoked when a workspace has been removed from the service
        /// </summary>
        /// <param name="connectionViewModel">removed workspace</param>
        public void OnWorkspaceRemoved(Workspace.IWorkspaceViewModel workspaceViewModel)
        {
            // Find root item
            Controls.IObservableTreeItem? item = Workspaces.FirstOrDefault(x => x.ViewModel == workspaceViewModel.PropertyCollection);

            // Found?
            if (item != null)
            {
                Workspaces.Remove(item);
                IsHelpVisible = Workspaces.Count == 0;
            }
        }

        /// <summary>
        /// Connect implementation
        /// </summary>
        private void OnConnect()
        {
            App.Locator.GetService<IWindowService>()?.OpenFor(new ConnectViewModel());
        }

        /// <summary>
        /// Invoked on refreshes 
        /// </summary>
        private void OnRefresh()
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Invoked on expansion
        /// </summary>
        private void OnExpand()
        {
            foreach (IObservableTreeItem workspace in Workspaces)
            {
                ExpandWorkspace(workspace, true);
            }
        }

        /// <summary>
        /// Invoked on workspace clears
        /// </summary>
        private void OnClearAll()
        {
            _workspaceService.Clear();
        }

        /// <summary>
        /// Invoked on collapses
        /// </summary>
        private void OnCollapse()
        {
            foreach (IObservableTreeItem workspace in Workspaces)
            {
                ExpandWorkspace(workspace, false);
            }
        }

        /// <summary>
        /// Set the expansion state of an item
        /// </summary>
        private void ExpandWorkspace(IObservableTreeItem workspace, bool expanded)
        {
            workspace.IsExpanded = expanded;

            // Handle children
            foreach (IObservableTreeItem item in workspace.Items)
            {
                ExpandWorkspace(item, expanded);
            }
        }
        
        /// <summary>
        /// Internal service
        /// </summary>
        private Services.IWorkspaceService _workspaceService;

        /// <summary>
        /// Internal help state
        /// </summary>
        private bool _isHelpVisible = true;
    }
}
