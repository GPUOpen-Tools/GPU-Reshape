// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
using System.Windows.Input;
using Avalonia;
using Avalonia.Media;
using Avalonia.Threading;
using Discovery.CLR;
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Setting;

namespace Studio.ViewModels.Controls
{
    public class DiscoveryDropdownViewModel : ReactiveObject
    {
        /// <summary>
        /// Current status color
        /// </summary>
        public IBrush? StatusColor
        {
            get => _statusColor;
            set => this.RaiseAndSetIfChanged(ref _statusColor, value);
        }

        /// <summary>
        /// Instance activation label
        /// </summary>
        public string InstanceLabel
        {
            get => _instanceLabel;
            set => this.RaiseAndSetIfChanged(ref _instanceLabel, value);
        }

        /// <summary>
        /// Discovery enabled globally
        /// </summary>
        public bool IsGloballyEnabled
        {
            get => _isGloballyEnabled;
            set => this.RaiseAndSetIfChanged(ref _isGloballyEnabled, value);
        }
        
        /// <summary>
        /// Toggle instance discovery
        /// </summary>
        public ICommand ToggleInstance { get; }
        
        /// <summary>
        /// Toggle global discovery
        /// </summary>
        public ICommand ToggleGlobal { get; }
        
        /// <summary>
        /// Constructor
        /// </summary>
        public DiscoveryDropdownViewModel()
        {
            // Get service
            _discoveryService = App.Locator.GetService<Services.IBackendDiscoveryService>()?.Service;
            
            // Create commands
            ToggleInstance = ReactiveCommand.Create(OnToggleInstance);
            ToggleGlobal = ReactiveCommand.Create(OnToggleGlobal);
            
            // Subscribe tick
            _timer.Tick += OnPool;
            
            // Initial update
            Update();
        }

        /// <summary>
        /// Invoked on global toggles
        /// </summary>
        private async void OnToggleGlobal()
        {
            // Bad service?
            if (_discoveryService == null)
            {
                return;
            }
            
            // Toggle
            if (_discoveryService.IsGloballyInstalled())
            {
                _discoveryService.UninstallGlobal();
            }
            else
            {
                // Global installation requires explicit user consent, reject if the VM could not be found (to be safe)
                if (_settingViewModel == null || await _settingViewModel.ConditionalWarning())
                {
                    // User has rejected, do not install
                    return;
                }
                
                // Consent has been granted, proceed
                _discoveryService.InstallGlobal();
            }
            
            // Manual refresh
            Update();
        }

        /// <summary>
        /// Invoked on instance toggles
        /// </summary>
        private void OnToggleInstance()
        {
            // Bad service?
            if (_discoveryService == null)
            {
                return;
            }
            
            // Toggle
            if (_discoveryService.IsRunning())
            {
                _discoveryService.Stop();
            }
            else
            {
                _discoveryService.Start();
            }
            
            // Manual refresh
            Update();
        }

        /// <summary>
        /// Invoked on pooling
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnPool(object? sender, EventArgs e)
        {
            Update();
        }

        /// <summary>
        /// Update all states
        /// </summary>
        private void Update()
        {
            // Bad service?
            if (_discoveryService == null)
            {
                StatusColor = new SolidColorBrush(ResourceLocator.GetResource<Color>("ErrorBrush"));
                return;
            }

            // Set status
            bool isRunning = _discoveryService.IsRunning();
            StatusColor = new SolidColorBrush(ResourceLocator.GetResource<Color>( isRunning? "SuccessColor" : "WarningColor"));
            InstanceLabel = isRunning ? "Stop discovery" : "Start discovery";

            // Set global status
            IsGloballyEnabled = _discoveryService.IsGloballyInstalled();
        }

        /// <summary>
        /// Internal status color
        /// </summary>
        private IBrush? _statusColor;
        
        /// <summary>
        /// Internal instance label
        /// </summary>
        private string _instanceLabel = "...";

        /// <summary>
        /// Internal timer for pooling
        /// </summary>
        private readonly DispatcherTimer _timer = new()
        {
            Interval = TimeSpan.FromSeconds(0.5),
            IsEnabled = true
        };

        /// <summary>
        /// Internal global state
        /// </summary>
        private bool _isGloballyEnabled;
        
        /// <summary>
        /// Backend discovery service
        /// </summary>
        private readonly DiscoveryService? _discoveryService;

        /// <summary>
        /// Setting view model
        /// </summary>
        private DiscoverySettingViewModel? _settingViewModel = AvaloniaLocator.Current.GetService<ISettingsService>()?.Get<DiscoverySettingViewModel>();
    }
}