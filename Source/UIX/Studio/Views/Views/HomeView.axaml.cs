using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Studio.Views.Views
{
    public partial class HomeView : UserControl
    {
        public HomeView()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}
