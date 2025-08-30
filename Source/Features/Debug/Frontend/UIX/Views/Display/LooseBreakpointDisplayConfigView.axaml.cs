using Avalonia.Controls;
using ReactiveUI;

namespace UIX.Views.Display;

public partial class LooseBreakpointDisplayConfigView : UserControl, IViewFor
{
    public object? ViewModel
    {
        get => DataContext;
        set => DataContext = value;
    }

    public LooseBreakpointDisplayConfigView()
    {
        InitializeComponent();
    }
}
