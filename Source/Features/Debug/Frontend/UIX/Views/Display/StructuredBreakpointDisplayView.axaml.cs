using System;
using Avalonia.Controls;
using GRS.Features.Debug.UIX.ViewModels;
using ReactiveUI;
using Studio.Extensions;

namespace UIX.Views.Display;

public partial class StructuredBreakpointDisplayView : UserControl, IViewFor
{
    public object? ViewModel
    {
        get => DataContext;
        set => DataContext = value;
    }

    public StructuredBreakpointDisplayView()
    {
        InitializeComponent();

        this.WhenAnyValue(x => x.DataContext)
            .CastNullable<StructuredBreakpointDisplayViewModel>()
            .Subscribe(viewModel =>
            {
                // Invalidate grid when data has changed
                viewModel.WhenAnyValue(x => x.DWords).Subscribe(_ =>
                {
                    Grid.InvalidateMeasure();
                    Grid.InvalidateVisual();
                });

                // Bind offsets
                Scroller.Events().ScrollChanged.Subscribe(e =>
                {
                    Grid.OffsetX = (float)Scroller.Offset.X;
                    Grid.OffsetY = (float)Scroller.Offset.Y;
                });
            }
        );
    }
}