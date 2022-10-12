using System;
using System.Diagnostics;
using System.Reactive.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using ReactiveUI;
using Runtime.ViewModels.Objects;
using Studio.Extensions;
using Studio.Models.Logging;
using Studio.Services;
using Studio.ViewModels.Tools;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.Views.Tools
{
    public partial class ShaderTreeView : UserControl
    {
        public ShaderTreeView()
        {
            InitializeComponent();

            this.WhenAnyValue(x => x.DataContext)
                .WhereNotNull()
                .Subscribe(x =>
                {
                    // Bind signals
                    ShaderList.Events().DoubleTapped
                        .Select(_ => ShaderList.SelectedItem)
                        .WhereNotNull()
                        .InvokeCommand(this, self => self.VM!.OpenShaderDocument);
                });
        }

        private void OnShaderIdentifierLayoutUpdated(object? sender, EventArgs e)
        {
            // Expected sender?
            if (sender is not Control { DataContext: ShaderIdentifierViewModel shaderIdentifierViewModel })
                return;

            // May already be pooled
            if (shaderIdentifierViewModel.HasBeenPooled)
                return;
            
            // Enqueue request
            VM?.PopulateSingle(shaderIdentifierViewModel);

            // Mark as pooled
            shaderIdentifierViewModel.HasBeenPooled = true;
        }
        
        private ShaderTreeViewModel? VM => DataContext as ShaderTreeViewModel;
    }
}
