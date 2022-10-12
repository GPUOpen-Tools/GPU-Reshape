using System;

namespace Runtime.Models.Objects
{
    public struct InstrumentationState
    {
        /// <summary>
        /// Instrumentation bit mask, see feature infos
        /// </summary>
        public UInt64 FeatureBitMask { get; set; }
    }
}