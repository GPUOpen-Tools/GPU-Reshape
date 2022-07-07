using System;

namespace Studio.Models.Workspace
{
    public class ApplicationInfo
    {
        /// <summary>
        /// Process id of the application
        /// </summary>
        public ulong Pid { get; set; }
        
        /// <summary>
        /// Decorative name of the application
        /// </summary>
        public string Name { get; set; }
        
        /// <summary>
        /// Process name of the application
        /// </summary>
        public string Process { get; set; }

        /// <summary>
        /// Host resolve GUID of the application
        /// </summary>
        public Guid Guid { get; set; }
    }
}