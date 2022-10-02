using System;
using System.Reactive.Linq;
using Avalonia;
using Bridge.CLR;
using DynamicData;
using DynamicData.Binding;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Logging;
using Studio.Services;
using Studio.ViewModels.Logging;
using Studio.ViewModels.Workspace;

namespace Studio.ViewModels.Status
{
    public class LogStatusViewModel : ReactiveObject, IStatusViewModel
    {
        /// <summary>
        /// Standard orientation
        /// </summary>
        public StatusOrientation Orientation => StatusOrientation.Left;

        /// <summary>
        /// Current status message to be displayed
        /// </summary>
        public string Status
        {
            get => _status;
            set => this.RaiseAndSetIfChanged(ref _status, value);
        }

        public LogStatusViewModel()
        {
            ILoggingViewModel? logging = AvaloniaLocator.Current.GetService<ILoggingService>()?.ViewModel;
            
            // Connect to workspaces
            logging?.Events.ToObservableChangeSet(x => x)
                .OnItemAdded(OnLog)
                .Subscribe();
        }

        /// <summary>
        /// Invoked on logging
        /// </summary>
        /// <param name="logEvent"></param>
        private void OnLog(LogEvent logEvent)
        {
            Status = logEvent.Message;
        }

        /// <summary>
        /// Internal status
        /// </summary>
        private string _status = string.Empty;
    }
}