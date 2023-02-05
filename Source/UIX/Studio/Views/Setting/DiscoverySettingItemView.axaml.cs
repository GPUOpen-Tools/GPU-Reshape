using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using ReactiveUI;

namespace Studio.Views.Setting
{
    public partial class DiscoverySettingItemView : UserControl, IViewFor
    {
        public object? ViewModel { get; set; }

        public DiscoverySettingItemView()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}