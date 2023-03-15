using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Workspace;
using System.Reactive.Linq;

namespace Studio.Views.Tools
{
    public partial class WorkspaceView : UserControl
    {
        public WorkspaceView()
        {
            InitializeComponent();
            
            this.WhenAnyValue(x => x.DataContext)
                .WhereNotNull()
                .Subscribe(x =>
                {
                    // Bind signals
                    WorkspaceTree.Events().DoubleTapped
                        .Select(_ => WorkspaceTree.SelectedItem as WorkspaceTreeItemViewModel)
                        .WhereNotNull()
                        .Subscribe(x => x.OpenDocument.Execute(null));
                });
        }

        private void OnItemSelected(object? sender, SelectionChangedEventArgs e)
        {
            // Validate sender
            if (e.AddedItems.Count == 0 || e.AddedItems[0] is not WorkspaceTreeItemViewModel { OwningContext: IWorkspaceViewModel workspaceViewModel })
                return;

            // Get service
            var service = App.Locator.GetService<IWorkspaceService>();
            if (service == null)
                return;

            // Set selected
            service.SelectedWorkspace = workspaceViewModel;
        }
        
        private WorkspaceViewModel? VM => DataContext as WorkspaceViewModel;
    }
}