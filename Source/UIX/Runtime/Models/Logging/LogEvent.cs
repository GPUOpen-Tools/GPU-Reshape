namespace Studio.Models.Logging
{
    public class LogEvent
    {
        /// <summary>
        /// Severity of this event
        /// </summary>
        public LogSeverity Severity { get; set; }

        /// <summary>
        /// Message of this event
        /// </summary>
        public string Message { get; set; } = string.Empty;
    }
}