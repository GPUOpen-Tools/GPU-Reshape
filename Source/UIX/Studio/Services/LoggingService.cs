using Studio.Models.Logging;
using Studio.ViewModels.Logging;

namespace Studio.Services
{
    public class LoggingService : ILoggingService
    {
        /// <summary>
        /// Hosted view model
        /// </summary>
        public ILoggingViewModel ViewModel { get; } = new LoggingViewModel();
    }
}