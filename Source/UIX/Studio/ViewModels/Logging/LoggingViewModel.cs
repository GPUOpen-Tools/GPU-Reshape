using System;
using System.Collections.ObjectModel;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Studio.Models.Logging;

namespace Studio.ViewModels.Logging
{
    public class LoggingViewModel : ReactiveObject, ILoggingViewModel
    {
        /// <summary>
        /// Current status string
        /// </summary>
        public string Status
        {
            get => _status;
            set => this.RaiseAndSetIfChanged(ref _status, value);
        }

        /// <summary>
        /// All events
        /// </summary>
        public SourceList<LogEvent> Events { get; } = new();

        /// <summary>
        /// Internal status string
        /// </summary>
        private string _status = String.Empty;
    }
}