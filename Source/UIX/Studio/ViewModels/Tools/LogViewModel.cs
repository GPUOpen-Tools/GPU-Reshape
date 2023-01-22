using System;
using System.Collections.ObjectModel;
using System.Windows.Input;
using Avalonia;
using Dock.Model.ReactiveUI.Controls;
using DynamicData;
using ReactiveUI;
using Studio.Models.Logging;
using Studio.Services;
using Studio.ViewModels.Logging;

namespace Studio.ViewModels.Tools
{
    public class LogViewModel : Tool
    {
        /// <summary>
        /// Refresh all items
        /// </summary>
        public ICommand Clear { get; }
        
        /// <summary>
        /// Toggle the info state
        /// </summary>
        public ICommand ToggleInfo { get; }

        /// <summary>
        /// Toggle the warning state
        /// </summary>
        public ICommand ToggleWarning { get; }

        /// <summary>
        /// Toggle the error state
        /// </summary>
        public ICommand ToggleError { get; }

        /// <summary>
        /// Should info messages be shown
        /// </summary>
        public bool IsShowInfo
        {
            get => _isShowInfo;
            set => this.RaiseAndSetIfChanged(ref _isShowInfo, value);
        }

        /// <summary>
        /// Should warning messages be shown
        /// </summary>
        public bool IsShowWarning
        {
            get => _isShowWarning;
            set => this.RaiseAndSetIfChanged(ref _isShowWarning, value);
        }

        /// <summary>
        /// Should error messages be shown
        /// </summary>
        public bool IsShowError
        {
            get => _isShowError;
            set => this.RaiseAndSetIfChanged(ref _isShowError, value);
        }

        /// <summary>
        /// Should error messages be shown
        /// </summary>
        public bool IsScrollLock
        {
            get => _isScrollLock;
            set => this.RaiseAndSetIfChanged(ref _isScrollLock, value);
        }

        /// <summary>
        /// Current set of filtered events
        /// </summary>
        public ReadOnlyObservableCollection<LogEvent>? FilteredEvents => _filteredEvents;

        /// <summary>
        /// Data view model
        /// </summary>
        public ILoggingViewModel? LoggingViewModel { get; }

        /// <summary>
        /// Constructor
        /// </summary>
        public LogViewModel()
        {
            LoggingViewModel = App.Locator.GetService<ILoggingService>()?.ViewModel;

            // Create commands
            Clear = ReactiveCommand.Create(OnClear);
            ToggleInfo = ReactiveCommand.Create(OnToggleInfo);
            ToggleWarning = ReactiveCommand.Create(OnToggleWarning);
            ToggleError = ReactiveCommand.Create(OnToggleError);

            // Create initial filter
            CreateFilter();
        }

        /// <summary>
        /// Create the internal filtering state
        /// </summary>
        private void CreateFilter()
        {
            LoggingViewModel?.Events.Connect()
                .Filter(x =>
                {
                    switch (x.Severity)
                    {
                        case LogSeverity.Info:
                            return _isShowInfo;
                        case LogSeverity.Warning:
                            return _isShowWarning;
                        case LogSeverity.Error:
                            return _isShowError;
                        default:
                            throw new ArgumentOutOfRangeException();
                    }
                })
                .Bind(out _filteredEvents)
                .DisposeMany()
                .Subscribe();
            
            // Notify any observers
            this.RaisePropertyChanged(nameof(FilteredEvents));
        }

        /// <summary>
        /// Invoked on clears
        /// </summary>
        private void OnClear()
        {
            LoggingViewModel?.Events.Clear();
        }

        /// <summary>
        /// Invoked on info toggling
        /// </summary>
        private void OnToggleInfo()
        {
            _isShowInfo = !_isShowInfo;
            CreateFilter();
        }

        /// <summary>
        /// Invoked on warning toggling
        /// </summary>
        private void OnToggleWarning()
        {
            _isShowWarning = !_isShowWarning;
            CreateFilter();
        }

        /// <summary>
        /// Invoked on error toggling
        /// </summary>
        private void OnToggleError()
        {
            _isShowError = !_isShowError;
            CreateFilter();
        }

        /// <summary>
        /// Internal filtered events
        /// </summary>
        private ReadOnlyObservableCollection<Models.Logging.LogEvent>? _filteredEvents;

        /// <summary>
        /// Internal info state
        /// </summary>
        private bool _isShowInfo = true;

        /// <summary>
        /// Internal warning state
        /// </summary>
        private bool _isShowWarning = true;

        /// <summary>
        /// Internal error state
        /// </summary>
        private bool _isShowError = true;

        /// <summary>
        /// Internal scroll state
        /// </summary>
        private bool _isScrollLock = true;
    }
}