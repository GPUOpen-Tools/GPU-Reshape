using System;

namespace Studio.ViewModels.Workspace.Listeners
{
    public interface IPulseService : IPropertyService
    {
        /// <summary>
        /// Last known pulse time
        /// </summary>
        public DateTime LastPulseTime { get; set; }
        
        /// <summary>
        /// Has the pulse been missed for an unreasonable amount of time?
        /// </summary>
        public bool MissedPulse { get; set; }
    }
}