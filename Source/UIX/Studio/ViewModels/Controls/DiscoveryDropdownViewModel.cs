using System;
using System.Windows.Input;
using Avalonia;
using Avalonia.Media;
using Avalonia.Threading;
using Discovery.CLR;
using ReactiveUI;

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
        private void OnToggleGlobal()
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
    }
}