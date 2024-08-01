using System;

namespace Studio.Models.Instrumentation
{
    [Flags]
    public enum InstrumentationFlag
    {
        /// <summary>
        /// No flags
        /// </summary>
        None = 0,
        
        /// <summary>
        /// This feature is part of the standard set of features
        /// </summary>
        Standard = 1,
        
        /// <summary>
        /// This feature is experimental
        /// </summary>
        Experimental = 2
    }
}