using System;
using System.Collections.Generic;
using Avalonia;
using Avalonia.Threading;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Logging;
using Studio.Services;
using Studio.ViewModels.Logging;

namespace Studio.ViewModels.Workspace.Properties
{
    public class LogViewModel : BasePropertyViewModel, Bridge.CLR.IBridgeListener
    {
        public LogViewModel() : base("Logs", PropertyVisibility.Default)
        {
            this.WhenAnyValue(x => x.ConnectionViewModel).Subscribe(_ => OnConnectionChanged());
        }
        
        /// <summary>
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            // Register to logging messages
            ConnectionViewModel?.Bridge?.Register(LogMessage.ID, this);

            // Create all properties
            CreateProperties();
        }

        /// <summary>
        /// Create all properties
        /// </summary>
        private void CreateProperties()
        {
            Properties.Clear();

            if (ConnectionViewModel == null)
            {
                return;
            }
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        /// <exception cref="NotImplementedException"></exception>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.GetSchema().IsDynamic(LogMessage.ID))
            {
                return;
            }

            List<string> messages = new();

            // Enumerate all messages
            foreach (LogMessage message in new DynamicMessageView<LogMessage>(streams))
            {
                messages.Add($"{ConnectionViewModel?.Application?.Name} - [{message.system.String}] {message.message.String}");
            }

            // Append messages on UI thread
            Dispatcher.UIThread.InvokeAsync(() =>
            {
                foreach (var message in messages)
                {
                    _loggingViewModel?.Events.Add(new LogEvent()
                    {
                        Severity = LogSeverity.Info,
                        Message = message
                    });
                }
            });
        }

        /// <summary>
        /// Shared logging view model
        /// </summary>
        private ILoggingViewModel? _loggingViewModel = App.Locator.GetService<ILoggingService>()?.ViewModel;
    }
}