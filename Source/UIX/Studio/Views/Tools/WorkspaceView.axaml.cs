using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Studio.Views.Tools
{
    public partial class WorkspaceView : UserControl
    {
        public WorkspaceView()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}
