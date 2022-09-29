namespace Studio.Models.Logging
{
    public enum LogSeverity
    {
        /// <summary>
        /// General information, can be discarded
        /// </summary>
        Info,
        
        /// <summary>
        /// Caution, some operation returned an unexpected state
        /// </summary>
        Warning,
        
        /// <summary>
        /// An operation has failed
        /// </summary>
        Error
    }
}