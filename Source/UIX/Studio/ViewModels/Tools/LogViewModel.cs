// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
using System.Windows.Input;
using Avalonia;
using Avalonia.Media;
using Dock.Model.ReactiveUI.Controls;
using DynamicData;
using ReactiveUI;
using Runtime.ViewModels.Tools;
using Studio.Models.Logging;
using Studio.Services;
using Studio.ViewModels.Logging;

namespace Studio.ViewModels.Tools
{
    public class LogViewModel : ToolViewModel
    {
        /// <summary>
        /// Tooling icon
        /// </summary>
        public override StreamGeometry? Icon => ResourceLocator.GetIcon("ToolLog");
        
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
        /// Toggle the error state
        /// </summary>
        public ICommand Open { get; }

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
            Open = ReactiveCommand.Create<LogEvent>(OnOpen);

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
        /// Invoked on view model handling
        /// </summary>
        private void OnOpen(LogEvent instance)
        {
            // Must have valid view model
            if (instance.ViewModel == null)
            {
                return;
            }
            
            // Open from derived
            if (App.Locator.GetService<IWindowService>() is { } service)
            {
                service.OpenFor(instance.ViewModel);
            }
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