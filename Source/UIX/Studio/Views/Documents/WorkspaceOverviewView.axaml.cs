using System;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Documents;

namespace Studio.Views.Documents
{
    public partial class WorkspaceOverviewView : UserControl
    {
        public WorkspaceOverviewView()
        {
            InitializeComponent();

            this.WhenAnyValue(x => x.DataContext).CastNullable<WorkspaceOverviewViewModel>().Subscribe(vm =>
            {
                DiagnosticPlot.DataContext = new DiagnosticPlotViewModel()
                {
                    Workspace = vm.Workspace
                };
            });
        }
    }
}
