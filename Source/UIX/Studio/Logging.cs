using Avalonia;
using Studio.Models.Logging;
using Studio.Services;
using Studio.ViewModels.Logging;

namespace Studio
{
    public class Logging
    {
        /// <summary>
        /// View model getter
        /// </summary>
        public static ILoggingViewModel? ViewModel => App.Locator.GetService<ILoggingService>()?.ViewModel;

        /// <summary>
        /// Add a new event log
        /// </summary>
        /// <param name="severity"></param>
        /// <param name="message"></param>
        public static void Add(LogSeverity severity, string message)
        {
            ViewModel?.Events.Add(new LogEvent()
            {
                Severity = severity,
                Message = message
            });
        }

        /// <summary>
        /// Add an info event log
        /// </summary>
        /// <param name="message"></param>
        public static void Info(string message)
        {
            Add(LogSeverity.Info, message);
        }

        /// <summary>
        /// Add a warning event log
        /// </summary>
        /// <param name="message"></param>
        public static void Warning(string message)
        {
            Add(LogSeverity.Warning, message);
        }

        /// <summary>
        /// Add an error event log
        /// </summary>
        /// <param name="message"></param>
        public static void Error(string message)
        {
            Add(LogSeverity.Error, message);
        }
    }
}