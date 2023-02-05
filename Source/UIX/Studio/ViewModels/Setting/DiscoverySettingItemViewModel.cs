using System;
using System.Collections.ObjectModel;
using System.Windows.Input;
using Avalonia;
using Avalonia.Threading;
using ReactiveUI;
using Studio.ViewModels.Setting;

namespace Studio.ViewModels.Setting
{
    public class DiscoverySettingItemViewModel : ReactiveObject, ISettingItemViewModel
    {
        /// <summary>
        /// Given header
        /// </summary>
        public string Header
        {
            get => _header;
            set => this.RaiseAndSetIfChanged(ref _header, value);
        }

        /// <summary>
        /// Is enabled?
        /// </summary>
        public bool IsEnabled
        {
            get => _isEnabled;
            set => this.RaiseAndSetIfChanged(ref _isEnabled, value);
        }

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
        /// Toggle the local state
        /// </summary>
        public ICommand LocalStateToggle { get; }

        /// <summary>
        /// Toggle the global state
        /// </summary>
        public ICommand GlobalStateToggle { get; }
        
        /// <summary>
        /// Items within
        /// </summary>
        public ObservableCollection<ISettingItemViewModel> Items { get; } = new();

        public DiscoverySettingItemViewModel()
        {
            // Create service
            _discoveryService = App.Locator.GetService<Services.IBackendDiscoveryService>()?.Service;
            
            // Set status on failure
            if (_discoveryService == null)
            {
                Status = "Failed initialization";
                ButtonText = "None";
            }
            
            // Create commands
            LocalStateToggle = ReactiveCommand.Create(OnLocalStateToggle);
            GlobalStateToggle = ReactiveCommand.Create(OnGlobalStateToggle);
            
            // Subscribe tick
            _timer.Tick += OnTick;
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
        private void OnGlobalStateToggle()
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
        /// Internal header
        /// </summary>
        private string _header = "Discovery";

        /// <summary>
        /// Internal enabled state
        /// </summary>
        private bool _isEnabled = true;
    }
}