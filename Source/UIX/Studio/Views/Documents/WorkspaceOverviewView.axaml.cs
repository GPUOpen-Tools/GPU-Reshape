using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Studio.Views.Documents
{
    public partial class WorkspaceOverviewView : UserControl
    {
        public WorkspaceOverviewView()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}
