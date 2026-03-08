using Avalonia.Controls;
using ReactiveUI;

namespace UIX.Views.Display;

public partial class LooseWatchpointDisplayConfigView : UserControl, IViewFor
{
    public object? ViewModel
    {
        get => DataContext;
        set => DataContext = value;
    }

    public LooseWatchpointDisplayConfigView()
    {
        InitializeComponent();
    }
}
