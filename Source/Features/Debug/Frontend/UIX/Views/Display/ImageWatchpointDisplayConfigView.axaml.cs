using System;
using Avalonia.Controls;
using GRS.Features.Debug.UIX.ViewModels;
using ReactiveUI;
using Studio.Extensions;

namespace UIX.Views.Display;

public partial class ImageWatchpointDisplayConfigView : UserControl, IViewFor
{
    public object? ViewModel
    {
        get => DataContext;
        set => DataContext = value;
    }

    public ImageWatchpointDisplayConfigView()
    {
        InitializeComponent();

        // Bind to events
        this.WhenAnyValue(x => x.DataContext)
            .CastNullable<ImageWatchpointDisplayViewModel>()
            .Subscribe(vm =>
            {
                AutoRange.Events().Click.Subscribe(_ =>
                {
                    if (vm.Inspector == null)
                    {
                        return;
                    }
        
                    // Summarize the range across the whole image
                    PixelValueRange range = vm.Inspector.SummarizeRange();
                    DisplayRangeSlider.Minimum = range.MinValue;
                    DisplayRangeSlider.LowerValue = range.MinValue;
                    DisplayRangeSlider.Maximum = range.MaxValue;
                    DisplayRangeSlider.UpperValue = range.MaxValue;
                });
            });
    }
}