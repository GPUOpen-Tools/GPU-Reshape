using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Controls;

namespace Studio.Views.Controls
{
    public partial class DiscoveryDropdown : UserControl
    {
        public DiscoveryDropdown()
        {
            InitializeComponent();

            // Bind checking
            this.GlobalCheckbox.Events().Click.ToSignal().InvokeCommand(((DiscoveryDropdownViewModel)DataContext!).ToggleGlobal);
        }
    }
}