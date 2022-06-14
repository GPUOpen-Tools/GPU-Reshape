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
        /// All workspaces
        /// </summary>
        public ObservableCollection<Controls.ObservableTreeItem> Workspaces { get; } = new();

        public WorkspaceViewModel()
        {
            Connect = ReactiveCommand.Create(OnConnect);

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
        /// <param name="connection">added workspace</param>
        public void OnWorkspaceAdded(WorkspaceConnection connection)
        {
            Workspaces.Add(new Controls.WorkspaceTreeItemViewModel()
            {
                Connection = connection
            });
        }

        /// <summary>
        /// Invoked when a workspace has been removed from the service
        /// </summary>
        /// <param name="connection">removed workspace</param>
        public void OnWorkspaceRemoved(WorkspaceConnection connection)
        {
            // Find root item
            Controls.ObservableTreeItem? item = Workspaces.FirstOrDefault(x => (x as Controls.WorkspaceTreeItemViewModel)?.Connection == connection);

            // Found?
            if (item != null)
            {
                Workspaces.Remove(item);
            }
        }

        /// <summary>
        /// Connect implementation
        /// </summary>
        private async void OnConnect()
        {
            // Must have desktop lifetime
            if (Avalonia.Application.Current?.ApplicationLifetime is not IClassicDesktopStyleApplicationLifetime desktop)
            {
                return;
            }

            // Create dialog
            var dialog = new ConnectWindow()
            {
                WindowStartupLocation = WindowStartupLocation.CenterOwner
            };

            // Blocking
            await dialog.ShowDialog(desktop.MainWindow);
        }

        /// <summary>
        /// Internal service
        /// </summary>
        private Services.IWorkspaceService _workspaceService;
    }
}
