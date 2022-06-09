using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Interactivity;
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

        private void OnClickConnect(object? sender, RoutedEventArgs e)
        {
            if (Avalonia.Application.Current?.ApplicationLifetime is not IClassicDesktopStyleApplicationLifetime desktop)
            {
                return;
            }

            // Create dialog
            var dialog = new ConnectWindow()
            {
                WindowStartupLocation = WindowStartupLocation.CenterOwner
            };

            // Blocking
            dialog.ShowDialog(desktop.MainWindow);
        }
    }
}
