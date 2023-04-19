using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.ExtendedToolkit.Controls.PropertyGrid.PropertyTypes;
using Avalonia.Interactivity;
using Avalonia.ReactiveUI;
using ReactiveUI;
using Studio.Extensions;
using Studio.Models.Tools;
using Studio.ViewModels;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;
using Studio.Views.Tools.Property;

namespace Studio.Views
{
    public partial class LaunchWindow : ReactiveWindow<ViewModels.ConnectViewModel>
    {
        public LaunchWindow()
        {
            InitializeComponent();
#if DEBUG
            this.AttachDevTools();
#endif

            // Bind events
            this.ApplicationButton.Events().Click.Subscribe(OnApplicationPathButton);
            this.WorkingDirectoryButton.Events().Click.Subscribe(OnWorkingDirectoryButton);
            this.WorkspaceTree.Events().SelectionChanged.Subscribe(OnWorkspaceSelectionChanged);

            // Bind data context
            this.WhenAnyValue(x => x.DataContext)
                .CastNullable<LaunchViewModel>()
                .Subscribe(x =>
                {
                    // Bind configurations
                    x.WhenAnyValue(y => y.SelectedPropertyConfigurations)
                        .Subscribe(y =>
                        {
                            // Create new descriptor
                            PropertyGrid.SelectedObject = new PropertyCollectionTypeDescriptor(y ?? Array.Empty<IPropertyViewModel>());

                            // Clear previous metadata, internally a type based cache is used, this is not appropriate
                            MetadataRepository.Clear();

                            // Finally, recreate the property setup with fresh metadata
                            PropertyGrid.ReloadCommand.Execute(null);
                        });
                    
                    // Bind interactions
                    x.AcceptLaunch.RegisterHandler(ctx =>
                    {
                        // Request closure
                        Close(true);
                        ctx.SetOutput(true);
                    });
                });
        }

        /// <summary>
        /// Invoked on selection changes
        /// </summary>
        /// <param name="e"></param>
        public void OnWorkspaceSelectionChanged(SelectionChangedEventArgs e)
        {
            // Validate sender
            if (e.AddedItems.Count == 0 || e.AddedItems[0] is not { } _object)
                return;
            
            // Workspace?
            if (_object is WorkspaceTreeItemViewModel { OwningContext: IWorkspaceViewModel workspaceViewModel })
            {
                _VM.SelectedProperty = workspaceViewModel.PropertyCollection;
            }

            // Property?
            if (_object is WorkspaceTreeItemViewModel { OwningContext: IPropertyViewModel propertyViewModel })
            {
                _VM.SelectedProperty = propertyViewModel;
            }
        }

        /// <summary>
        /// Invoked on application button
        /// </summary>
        /// <param name="x"></param>
        private async void OnApplicationPathButton(RoutedEventArgs x)
        {
            var dialog = new OpenFileDialog();

            string[]? result = await dialog.ShowAsync(this);
            if (result == null)
            {
                return;
            }

            _VM.ApplicationPath = result[0];
        }

        /// <summary>
        /// Invoked on directory button
        /// </summary>
        /// <param name="x"></param>
        private async void OnWorkingDirectoryButton(RoutedEventArgs x)
        {
            var dialog = new OpenFileDialog();

            string[]? result = await dialog.ShowAsync(this);
            if (result == null)
            {
                return;
            }

            _VM.WorkingDirectoryPath = result[0];
        }

        private LaunchViewModel _VM => (LaunchViewModel)DataContext!;
    }
}