using Avalonia;
using Avalonia.Controls;

namespace UIX.Views;

public partial class BreakpointWindow : Window
{
    public BreakpointWindow()
    {
        InitializeComponent();
        
#if DEBUG
        this.AttachDevTools();
#endif // DEBUG
    }
}