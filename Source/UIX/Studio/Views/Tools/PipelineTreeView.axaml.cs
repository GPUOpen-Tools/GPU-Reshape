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
    public partial class PipelineTreeView : UserControl
    {
        public PipelineTreeView()
        {
            InitializeComponent();

            this.WhenAnyValue(x => x.DataContext)
                .WhereNotNull()
                .Subscribe(x =>
                {
                    // Bind signals
                    PipelineList.Events().DoubleTapped
                        .Select(_ => PipelineList.SelectedItem)
                        .WhereNotNull()
                        .InvokeCommand(this, self => self.VM!.OpenPipelineDocument);
                });
        }

        private void OnPipelineIdentifierLayoutUpdated(object? sender, EventArgs e)
        {
            // Expected sender?
            if (sender is not Control { DataContext: PipelineIdentifierViewModel pipelineIdentifierViewModel })
                return;

            // May already be pooled
            if (pipelineIdentifierViewModel.HasBeenPooled)
                return;
            
            // Enqueue request
            VM?.PopulateSingle(pipelineIdentifierViewModel);

            // Mark as pooled
            pipelineIdentifierViewModel.HasBeenPooled = true;
        }
        
        private PipelineTreeViewModel? VM => DataContext as PipelineTreeViewModel;
    }
}
