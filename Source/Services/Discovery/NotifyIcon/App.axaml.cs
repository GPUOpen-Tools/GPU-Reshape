using System;
using System.Diagnostics;
using System.Globalization;
using System.Threading;
using System.Windows.Input;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Data;
using Avalonia.Markup.Xaml;
using Avalonia.Markup.Xaml.Styling;
using Avalonia.Styling;
using Avalonia.Threading;
using ReactiveUI;

namespace Studio
{
    public class App : Application
    {
        public static IAvaloniaDependencyResolver Locator = AvaloniaLocator.Current;
        
        /// <summary>
        /// Open GPUReshape command
        /// </summary>
        public ICommand OpenCommand { get; }
        
        /// <summary>
        /// Stop discovery command
        /// </summary>
        public ICommand StopCommand { get; }

        /// <summary>
        /// Constructor
        /// </summary>
        public App()
        {
            // Create service
            _service = new Discovery.CLR.DiscoveryService();

            // Try to install
            if (!_service.Install())
            {
                MessageBox.Avalonia.MessageBoxManager.GetMessageBoxStandardWindow(
                    "GPU Reshape",
                    "Failed to initialize discovery service"
                ).Show();

                // Request shutdown
                Stop();

                // No need to continue
                return;
            }
            
            // Create commands
            OpenCommand = ReactiveCommand.Create(OnOpen);
            StopCommand = ReactiveCommand.Create(OnStop);
            
            // Subscribe
            _timer.Tick += OnPool;
        }

        /// <summary>
        /// Invoked on timer pooling
        /// </summary>
        private void OnPool(object? sender, EventArgs e)
        {
            // If either are valid, the service is enabled
            if (_service.IsRunning() || _service.IsGloballyInstalled())
            {
                return;
            }

            // Request shutdown
            Stop();
        }

        /// <summary>
        /// Invoked on application open
        /// </summary>
        private void OnOpen()
        {
            // Find all processes of name
            Process[] processes = Process.GetProcessesByName("GPUReshape");

            // Any?
            if (processes.Length > 0)
            {
                IntPtr handle = processes[0].MainWindowHandle;
                
                // Restore if needed
                if (IsIconic(handle))
                {
                    const int restoreId = 9;
                    ShowWindow(handle, restoreId);
                }

                // Bring to focus
                SetForegroundWindow(handle);
            }
            else
            {
                // None found, start new process
                new Process()
                {
                    StartInfo = new ProcessStartInfo()
                    {
                        FileName = "GPUReshape.exe"
                    }
                }.Start();
            }
        }

        /// <summary>
        /// Invoked on discovery stop
        /// </summary>
        private void OnStop()
        {
            // Try to uninstall and stop
            if (!_service.UninstallGlobal() || !_service.Stop())
            {
                MessageBox.Avalonia.MessageBoxManager.GetMessageBoxStandardWindow(
                    "GPU Reshape",
                    "Failed to stop and uninstall instances"
                ).Show();
                
                // Can't continue
                return;
            }

            // Request shutdown
            Stop();
        }

        /// <summary>
        /// Invoked on initialization
        /// </summary>
        public override void Initialize()
        {
            // Set culture
            Thread.CurrentThread.CurrentUICulture = CultureInfo.GetCultureInfo("en");

            // Load app
            AvaloniaXamlLoader.Load(this);
        }

        /// <summary>
        /// Stop the application
        /// </summary>
        private void Stop()
        {
            if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktopStyleApplicationLifetime)
            {
                desktopStyleApplicationLifetime.Shutdown();
            }
        }

        /// <summary>
        /// Pooling timer, defaulted to every 3 seconds
        /// </summary>
        private DispatcherTimer _timer = new()
        {
            Interval = TimeSpan.FromSeconds(3),
            IsEnabled = true
        };

        private Discovery.CLR.DiscoveryService _service;
        
        // https://stackoverflow.com/questions/2636721/bring-another-processes-window-to-foreground-when-it-has-showintaskbar-false
        
        [System.Runtime.InteropServices.DllImport("User32.dll")]
        private static extern bool SetForegroundWindow(IntPtr handle);
        
        [System.Runtime.InteropServices.DllImport("User32.dll")]
        private static extern bool ShowWindow(IntPtr handle, int nCmdShow);
        
        [System.Runtime.InteropServices.DllImport("User32.dll")]
        private static extern bool IsIconic(IntPtr handle);
    }
}