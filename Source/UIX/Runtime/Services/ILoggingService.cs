using Studio.ViewModels.Logging;

namespace Studio.Services
{
    public interface ILoggingService
    {
        /// <summary>
        /// Hosted view model
        /// </summary>
        public ILoggingViewModel ViewModel { get; }
    }
}