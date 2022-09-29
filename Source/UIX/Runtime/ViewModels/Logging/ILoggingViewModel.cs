using System.Collections.ObjectModel;

namespace Studio.ViewModels.Logging
{
    public interface ILoggingViewModel
    {
        /// <summary>
        /// Current status string
        /// </summary>
        public string Status { get; set; }
        
        /// <summary>
        /// All events
        /// </summary>
        public ObservableCollection<Models.Logging.LogEvent> Events { get; }
    }
}