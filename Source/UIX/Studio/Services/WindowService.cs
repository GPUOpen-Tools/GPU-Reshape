using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Runtime.ViewModels;
using Studio.Views;

namespace Studio.Services
{
    public class WindowService : IWindowService
    {
        /// <summary>
        /// Shared layout
        /// </summary>
        public ILayoutViewModel? LayoutViewModel { get; set; } = null;

        /// <summary>
        /// Open a window for a given view model
        /// </summary>
        /// <param name="viewModel">view model</param>
        public void OpenFor(object viewModel)
        {
            // Must have desktop lifetime
            if (Application.Current?.ApplicationLifetime is not IClassicDesktopStyleApplicationLifetime desktop)
            {
                return;
            }

            // Attempt to resolve
            Window? window = _locator.ResolveWindow(viewModel);
            if (window == null)
            {
                return;
            }
            
            // Properties
            window.WindowStartupLocation = WindowStartupLocation.CenterOwner;

            // Blocking
            window.ShowDialog(desktop.MainWindow);
        }

        /// <summary>
        /// Request exit
        /// </summary>
        public void Exit()
        {
            if (Application.Current?.ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
            {
                desktop.MainWindow.Close();
            }
        }

        /// <summary>
        /// Internal locator
        /// </summary>
        private UniformWindowLocator _locator = new();
    }
}