using Avalonia.Controls;
using ReactiveUI;

namespace UIX.Views.Display;

public partial class ImageBreakpointDisplayView : UserControl, IViewFor
{
    public object? ViewModel
    {
        get => DataContext;
        set => DataContext = value;
    }

    public ImageBreakpointDisplayView()
    {
        InitializeComponent();
    }
}