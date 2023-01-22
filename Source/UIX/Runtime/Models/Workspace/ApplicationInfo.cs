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
        /// Underlying API of the application
        /// </summary>
        public string API { get; set; } = string.Empty;
        
        /// <summary>
        /// Decorative name of the application
        /// </summary>
        public string Name { get; set; } = string.Empty;
        
        /// <summary>
        /// Process name of the application
        /// </summary>
        public string Process { get; set; } = string.Empty;

        /// <summary>
        /// Host resolve GUID of the application
        /// </summary>
        public Guid Guid { get; set; }

        /// <summary>
        /// Decorated name of the info
        /// </summary>
        public string DecoratedName => $"{Process} - {Name}";
    }
}