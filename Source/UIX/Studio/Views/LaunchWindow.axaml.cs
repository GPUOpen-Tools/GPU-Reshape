using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.ReactiveUI;
using Studio.ViewModels;

namespace Studio.Views
{
    public partial class LaunchWindow : ReactiveWindow<ViewModels.ConnectViewModel>
    {
        public LaunchWindow()
        {
            InitializeComponent();
#if DEBUG
            this.AttachDevTools();
#endif

            // Bind events
            this.ApplicationButton.Events().Click.Subscribe(OnApplicationPathButton);
            this.WorkingDirectoryButton.Events().Click.Subscribe(OnWorkingDirectoryButton);
        }

        /// <summary>
        /// Invoked on application button
        /// </summary>
        /// <param name="x"></param>
        private async void OnApplicationPathButton(RoutedEventArgs x)
        {
            var dialog = new OpenFileDialog();

            string[]? result = await dialog.ShowAsync(this);
            if (result == null)
            {
                return;
            }

            _VM.ApplicationPath = result[0];
        }

        /// <summary>
        /// Invoked on directory button
        /// </summary>
        /// <param name="x"></param>
        private async void OnWorkingDirectoryButton(RoutedEventArgs x)
        {
            var dialog = new OpenFileDialog();

            string[]? result = await dialog.ShowAsync(this);
            if (result == null)
            {
                return;
            }

            _VM.WorkingDirectoryPath = result[0];
        }

        private LaunchViewModel _VM => (LaunchViewModel)DataContext!;
    }
}