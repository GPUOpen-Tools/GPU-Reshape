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
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using System.Windows.Input;
using Avalonia;
using Avalonia.Threading;
using DynamicData;
using ReactiveUI;
using Studio.Models.Logging;
using Studio.Services;
using Studio.Services.Suspension;
using Studio.ViewModels.Setting;

namespace Studio.ViewModels.Setting
{
    public class DiscoverySettingViewModel : BaseSettingViewModel
    {
        /// <summary>
        /// Current status text
        /// </summary>
        public string Status
        {
            get => _status;
            set => this.RaiseAndSetIfChanged(ref _status, value);
        }
        
        /// <summary>
        /// Local button text
        /// </summary>
        public string ButtonText
        {
            get => _buttonText;
            set => this.RaiseAndSetIfChanged(ref _buttonText, value);
        }

        /// <summary>
        /// Is the service running?
        /// </summary>
        public bool IsRunning
        {
            get => _isRunning;
            set
            {
                this.RaiseAndSetIfChanged(ref _isRunning, value);

                // Update text
                Status = value ? "Running" : "Stopped";
                ButtonText = value ? "Stop" : "Start";
            }
        }

        /// <summary>
        /// Are there any conflicting instances running?
        /// </summary>
        public bool HasConflictingInstances
        {
            get => _hasConflictingInstances;
            set => this.RaiseAndSetIfChanged(ref _hasConflictingInstances, value);
        }

        /// <summary>
        /// Is the service globally installed?
        /// </summary>
        public bool IsGloballyInstalled
        {
            get => _isGloballyInstalled;
            set
            {
                this.RaiseAndSetIfChanged(ref _isGloballyInstalled, value);

                if (value)
                {
                    Status = "Globally installed";
                }
            }
        }
        
        /// <summary>
        /// Should the user be warned on global installation?
        /// </summary>
        [DataMember]
        public bool WarnOnGlobalInstall
        {
            get => _warnOnGlobalInstall;
            set => this.RaiseAndSetIfChanged(ref _warnOnGlobalInstall, value);
        }

        /// <summary>
        /// Toggle the local state
        /// </summary>
        public ICommand LocalStateToggle { get; }

        /// <summary>
        /// Toggle the global state
        /// </summary>
        public ICommand GlobalStateToggle { get; }

        /// <summary>
        /// Toggle the local state
        /// </summary>
        public ICommand CleanConflictingInstances { get; }

        public DiscoverySettingViewModel() : base("Discovery")
        {
            // Create service
            _discoveryService = App.Locator.GetService<IBackendDiscoveryService>()?.Service;
            
            // Set status on failure
            if (_discoveryService != null)
            {
                HasConflictingInstances = _discoveryService.HasConflictingInstances();
            }
            else
            {
                Status = "Failed initialization";
                ButtonText = "None";
            }
            
            // Create commands
            LocalStateToggle = ReactiveCommand.Create(OnLocalStateToggle);
            GlobalStateToggle = ReactiveCommand.Create(OnGlobalStateToggle);
            CleanConflictingInstances = ReactiveCommand.Create(OnCleanConflictingInstances);
            
            // Subscribe tick
            _timer.Tick += OnTick;
        }

        /// <summary>
        /// Start a conditional warning dialog, does not run if previously marked as hidden
        /// </summary>
        public async Task<bool> ConditionalWarning()
        {
            // Don't warn?
            if (!WarnOnGlobalInstall)
            {
                return false;
            }
            
            // Show window
            var vm = await AvaloniaLocator.Current.GetService<IWindowService>()!.OpenFor<DialogViewModel>(new DialogViewModel()
            {
                Title = Resources.Resources.Discovery_Warning_Title,
                Content = Resources.Resources.Discovery_Warning_Content,
                ShowHideNextTime = true
            });

            // Failed to open?
            if (vm == null)
            {
                return false;
            }

            // User requested not to warn next time?
            if (vm.HideNextTime)
            {
                WarnOnGlobalInstall = false;
            }
            
            // OK
            return !vm.Result;
        }

        /// <summary>
        /// Invoked on conflict cleans
        /// </summary>
        private void OnCleanConflictingInstances()
        {
            // Try to uninstall
            if (!(_discoveryService?.UninstallConflictingInstances() ?? false))
            {
                App.Locator.GetService<ILoggingService>()?.ViewModel.Events.Add(new LogEvent()
                {
                    Severity = LogSeverity.Error,
                    Message = "Failed to uninstall conflicting discovery instances"
                });

                // Something went wrong...
                return;
            }

            // Mark as OK
            HasConflictingInstances = false;
        }

        /// <summary>
        /// Invoked on tick
        /// </summary>
        private void OnTick(object? sender, EventArgs e)
        {
            if (_discoveryService == null)
            {
                return;
            }

            IsRunning = _discoveryService.IsRunning();
            IsGloballyInstalled = _discoveryService.IsGloballyInstalled();
        }

        /// <summary>
        /// Invoked on local toggles
        /// </summary>
        private void OnLocalStateToggle()
        {
            if (_discoveryService == null)
            {
                return;
            }
            
            if (_isRunning)
            {
                _discoveryService.Stop();
            }
            else
            {
                _discoveryService.Start();
            }
        }

        /// <summary>
        /// Invoked on global toggles
        /// </summary>
        private async void OnGlobalStateToggle()
        {
            if (_discoveryService == null)
            {
                return;
            }
            
            if (_discoveryService.IsGloballyInstalled())
            {
                _discoveryService.UninstallGlobal();
            }
            else
            {
                // Global installation requires explicit user consent
                if (await ConditionalWarning())
                {
                    // User has rejected, do not install
                    return;
                }
                
                // Consent has been granted, proceed
                _discoveryService.InstallGlobal();
            }
        }

        /// <summary>
        /// Internal timer for pooling
        /// </summary>
        private DispatcherTimer _timer = new()
        {
            Interval = TimeSpan.FromSeconds(0.5),
            IsEnabled = true
        };

        /// <summary>
        /// Underlying service
        /// </summary>
        private Discovery.CLR.DiscoveryService? _discoveryService;

        /// <summary>
        /// Internal running state
        /// </summary>
        private bool _isRunning = false;

        /// <summary>
        /// Internal global state
        /// </summary>
        private bool _isGloballyInstalled = false;

        /// <summary>
        /// Internal status state
        /// </summary>
        private string _status = "Pooling...";

        /// <summary>
        /// Internal button text state
        /// </summary>
        private string _buttonText = "...";

        /// <summary>
        /// Internal conflicting state
        /// </summary>
        private bool _hasConflictingInstances = false;

        /// <summary>
        /// Internal warning state
        /// </summary>
        private bool _warnOnGlobalInstall = true;
    }
}