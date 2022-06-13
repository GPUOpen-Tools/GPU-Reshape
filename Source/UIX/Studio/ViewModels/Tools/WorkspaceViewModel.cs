using System.Windows.Input;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Interactivity;
using Dock.Model.ReactiveUI.Controls;
using ReactiveUI;
using Studio.Views;

namespace Studio.ViewModels.Tools
{
    public class WorkspaceViewModel : Tool
    {
        /// <summary>
        /// Connect to new workspace
        /// </summary>
        public ICommand Connect { get; }

        public WorkspaceViewModel()
        {
            Connect = ReactiveCommand.Create(OnConnect);
        }

        /// <summary>
        /// Connect implementation
        /// </summary>
        private async void OnConnect()
        {
            // Must have desktop lifetime
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
            await dialog.ShowDialog(desktop.MainWindow);
        }
    }
}
