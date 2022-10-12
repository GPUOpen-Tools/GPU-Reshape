using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Studio.Services;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Workspace;

namespace Studio.Views.Tools
{
    public partial class WorkspaceView : UserControl
    {
        public WorkspaceView()
        {
            InitializeComponent();
        }

        private void OnItemSelected(object? sender, SelectionChangedEventArgs e)
        {
            // Validate sender
            if (sender is not Control { DataContext: WorkspaceTreeItemViewModel { OwningContext: IWorkspaceViewModel workspaceViewModel } })
                return;

            // Get service
            var service = App.Locator.GetService<IWorkspaceService>();
            if (service == null)
                return;

            // Set selected
            service.SelectedWorkspace = workspaceViewModel;
        }
    }
}