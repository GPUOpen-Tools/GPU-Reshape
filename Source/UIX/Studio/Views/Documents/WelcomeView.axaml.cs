using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Studio.Views.Documents
{
    public partial class WelcomeView : UserControl
    {
        public WelcomeView()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}
