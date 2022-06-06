using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Studio.Views.Tools
{
    public partial class ConnectionView : UserControl
    {
        public ConnectionView()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}
