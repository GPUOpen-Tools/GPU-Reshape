using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Traits;

namespace Studio.Views.Controls
{
    public partial class WorkspaceTreeItemView : UserControl, IViewFor
    {
        /// <summary>
        /// Assigned view model
        /// </summary>
        public object? ViewModel { get => DataContext; set => DataContext = value; }
        
        public WorkspaceTreeItemView()
        {
            InitializeComponent();

            // Bind on context
            this.WhenAnyValue(x => x.DataContext)
                .CastNullable<WorkspaceTreeItemViewModel>()
                .Subscribe(itemViewModel =>
                {
                    // React to instrumentation changes
                    if (itemViewModel.ViewModel is IInstrumentableObject instrumentableObject)
                    {
                        instrumentableObject.WhenAnyValue(x => x.InstrumentationState).Subscribe(_ =>
                        {
                            itemViewModel.RaisePropertyChanged(nameof(itemViewModel.ViewModel));
                        });
                    }
                });
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}