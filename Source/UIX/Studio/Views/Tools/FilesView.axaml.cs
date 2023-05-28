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
using Runtime.ViewModels.Shader;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.Views.Tools
{
    public partial class FilesView : UserControl
    {
        public FilesView()
        {
            InitializeComponent();
            
            this.WhenAnyValue(x => x.DataContext)
                .WhereNotNull()
                .Subscribe(x =>
                {
                    // Bind signals
                    FilesTree.Events().SelectionChanged.Subscribe(e => OnItemSelected(this, e));
                });
        }

        private void OnItemSelected(object? sender, SelectionChangedEventArgs e)
        {
            // Validate sender
            if (e.AddedItems.Count == 0 || e.AddedItems[0] is not { } _object)
                return;
            
            // Get service
            var service = App.Locator.GetService<IWorkspaceService>();
            if (service?.SelectedShader == null)
                return;

            // Shader file?
            if (_object is FileTreeItemViewModel { ViewModel: ShaderFileViewModel shaderFileViewModel})
            {
                service.SelectedShader.SelectedFile = shaderFileViewModel;
            }
        }
    }
}