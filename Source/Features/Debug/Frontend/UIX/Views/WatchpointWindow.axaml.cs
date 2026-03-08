using Avalonia;
using Avalonia.Controls;

namespace UIX.Views;

public partial class WatchpointWindow : Window
{
    public WatchpointWindow()
    {
        InitializeComponent();
        
#if DEBUG
        this.AttachDevTools();
#endif // DEBUG
    }
}