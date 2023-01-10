using System;

namespace Studio.Models.Workspace.Objects
{
    public struct ShaderLocation
    {
        /// <summary>
        /// Originating shader UID
        /// </summary>
        public UInt64 SGUID { get; set; }
        
        /// <summary>
        /// Line offset
        /// </summary>
        public int Line { get; set; }
        
        /// <summary>
        /// File identifier
        /// </summary>
        public int FileUID { get; set; }

        /// <summary>
        /// Column offset
        /// </summary>
        public int Column { get; set; }
    }
}