using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Studio.Views.Tools
{
    public partial class LogView : UserControl
    {
        public LogView()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}
