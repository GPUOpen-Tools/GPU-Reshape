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
using Studio.ViewModels.Workspace.Properties;

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
            if (e.AddedItems.Count == 0 || e.AddedItems[0] is not { } _object)
                return;
            
            // Get service
            var service = App.Locator.GetService<IWorkspaceService>();
            if (service == null)
                return;

            // Workspace?
            if (_object is WorkspaceTreeItemViewModel { OwningContext: IWorkspaceViewModel workspaceViewModel })
            {
                service.SelectedWorkspace = workspaceViewModel;
                service.SelectedProperty = workspaceViewModel.PropertyCollection;
            }

            // Property?
            if (_object is WorkspaceTreeItemViewModel { OwningContext: IPropertyViewModel propertyViewModel })
            {
                service.SelectedProperty = propertyViewModel;
            }
        }
        
        private WorkspaceViewModel? VM => DataContext as WorkspaceViewModel;
    }
}