// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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
using MsBox.Avalonia;
using ReactiveUI;

namespace Studio
{
    public class App : Application
    {
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
                MessageBoxManager.GetMessageBoxStandard(
                    "GPU Reshape",
                    "Failed to initialize discovery service"
                ).ShowAsync().Wait();

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
                if (Win32.IsIconic(handle))
                {
                    Win32.ShowWindow(handle, Win32.CmdShowRestoreId);
                }

                // Bring to focus
                Win32.SetForegroundWindow(handle);
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
                MessageBoxManager.GetMessageBoxStandard(
                    "GPU Reshape",
                    "Failed to stop and uninstall instances"
                ).ShowAsync().Wait();
                
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
    }
}