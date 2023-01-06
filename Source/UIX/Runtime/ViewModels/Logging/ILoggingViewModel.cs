using System.Collections.ObjectModel;
using DynamicData;

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
        public SourceList<Models.Logging.LogEvent> Events { get; }
    }
}