using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Studio.Views.Tools
{
    public partial class PropertyView : UserControl
    {
        public PropertyView()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}
